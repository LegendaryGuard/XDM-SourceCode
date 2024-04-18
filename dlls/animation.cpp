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

#include "extdll.h"// XDM: min/max funcs
#include "enginecallback.h"
#include "util_vector.h"
#include "util.h"
#include "activity.h"
#include "activitymap.h"
#include "studio.h"
#include "animation.h"
#include "scriptevent.h"

// XDM: NOTE: this code is VERY old
int ActivityMapFind(Activity iActivity)
{
	int i = 0;
	while (activity_map[i].type != 0)
	{
		if (activity_map[i].type == iActivity)
			break;

		++i;
	}
	return i;// should be last "terminator" activity if not found
}

mstudioseqdesc_t *GetSequenceData(void *pmodel, int sequence)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return NULL;
	if (sequence >= pstudiohdr->numseq)
		return NULL;

	return (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + sequence;
}

int ExtractBbox(void *pmodel, int sequence, Vector &mins, Vector &maxs)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return 0;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);
	if (pseqdesc)
	{
		mins = pseqdesc[sequence].bbmin;
		maxs = pseqdesc[sequence].bbmax;
		return 1;
	}
	return 0;
}


int LookupActivity( void *pmodel, entvars_t *pev, int activity )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return 0;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

	int weighttotal = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	for (int i = 0; i < pstudiohdr->numseq; ++i)
	{
		if (pseqdesc[i].activity == activity)
		{
			weighttotal += pseqdesc[i].actweight;
			if (!weighttotal || RANDOM_LONG(0,weighttotal-1) < pseqdesc[i].actweight)
				seq = i;
		}
	}
	return seq;
}


int LookupActivityHeaviest( void *pmodel, entvars_t *pev, int activity )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return 0;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);
	int weight = 0;
	int seq = ACTIVITY_NOT_AVAILABLE;
	for (int i = 0; i < pstudiohdr->numseq; ++i)
	{
		if (pseqdesc[i].activity == activity)
		{
			if (pseqdesc[i].actweight > weight)
			{
				weight = pseqdesc[i].actweight;
				seq = i;
			}
		}
	}
	return seq;
}

void GetEyePosition(void *pmodel, Vector &vecEyePosition)
{
	if (pmodel)
		vecEyePosition = ((studiohdr_t *)pmodel)->eyeposition;
}

// Returns -1 on failure
int LookupSequence( void *pmodel, const char *label )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return 0;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);
	for (int i = 0; i < pstudiohdr->numseq; ++i)
	{
		if (_stricmp(pseqdesc[i].label, label) == 0)
			return i;
	}
	return -1;
}

int IsSoundEvent( int eventNumber )
{
	if ( eventNumber == SCRIPT_EVENT_SOUND || eventNumber == SCRIPT_EVENT_SOUND_VOICE )
		return 1;
	return 0;
}

void SequencePrecache(void *pmodel, const char *pSequenceName)
{
	int index = LookupSequence(pmodel, pSequenceName);
	if (index >= 0)
	{
		studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
		if (!pstudiohdr || index >= pstudiohdr->numseq)
			return;

		mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + index;
		mstudioevent_t *pevent = (mstudioevent_t *)((byte *)pstudiohdr + pseqdesc->eventindex);
		for (int i = 0; i < pseqdesc->numevents; ++i)
		{
			// Don't send client-side events to the server AI
			if (pevent[i].event >= EVENT_CLIENT)
				continue;

			// UNDONE: Add a callback to check to see if a sound is precached yet and don't allocate a copy
			// of it's name if it is.
			if (IsSoundEvent(pevent[i].event))
			{
				if (strlen(pevent[i].options) == 0)
					conprintf(0, "Bad sound event %d in sequence %s :: %s (sound \"%s\")\n", pevent[i].event, pstudiohdr->name, pSequenceName, pevent[i].options);

				//PRECACHE_SOUND((char *)(gpGlobals->pStringBase + ALLOC_STRING(pevent[i].options)));
				PRECACHE_SOUND(STRINGV(ALLOC_STRING(pevent[i].options)));
			}
		}
	}
}

int GetSequenceInfo(void *pmodel, entvars_t *pev, float *pflFrameRate, float *pflGroundSpeed)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL || pev == NULL)
		return 0;

	if (pev->sequence >= pstudiohdr->numseq)
		return 0;

	mstudioseqdesc_t *pseqdesc = GetSequenceData(pmodel, pev->sequence);

	if (pseqdesc->numframes > 1)
	{
		*pflFrameRate = 256.0f * pseqdesc->fps/(pseqdesc->numframes - 1);
		*pflGroundSpeed = sqrt( pseqdesc->linearmovement[0]*pseqdesc->linearmovement[0]+ pseqdesc->linearmovement[1]*pseqdesc->linearmovement[1]+ pseqdesc->linearmovement[2]*pseqdesc->linearmovement[2] );
		*pflGroundSpeed = *pflGroundSpeed * pseqdesc->fps / (pseqdesc->numframes - 1);
	}
	else
	{
		*pflFrameRate = 256.0f;
		*pflGroundSpeed = 0.0f;
	}
	return 1;
}

int GetSequenceFlags(void *pmodel, entvars_t *pev)
{
	mstudioseqdesc_t *pseqdesc = GetSequenceData(pmodel, pev->sequence);
	if (pseqdesc)
		return pseqdesc->flags;

	return 0;
}

int GetAnimationEvent(void *pmodel, entvars_t *pev, MonsterEvent_t *pMonsterEvent, float flStart, float flEnd, int index)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL || pev->sequence >= pstudiohdr->numseq || !pMonsterEvent)
		return 0;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + (int)pev->sequence;
	if (pseqdesc->numevents == 0 || index > pseqdesc->numevents)
		return 0;

	if (pseqdesc->numframes > 1)
	{
		flStart *= (pseqdesc->numframes - 1) / 256.0f;
		flEnd *= (pseqdesc->numframes - 1) / 256.0f;
	}
	else
	{
		flStart = 0.0f;
		flEnd = 1.0f;
	}

	mstudioevent_t *pevent = (mstudioevent_t *)((byte *)pstudiohdr + pseqdesc->eventindex);

	// start/continue searching
	for (; index < pseqdesc->numevents; ++index)
	{
		// Don't send client-side events to the server AI
		if (pevent[index].event >= EVENT_CLIENT)
			continue;

		if ((pevent[index].frame >= flStart && pevent[index].frame < flEnd) ||
			((pseqdesc->flags & STUDIO_LOOPING) && flEnd >= pseqdesc->numframes - 1 && pevent[index].frame < flEnd - pseqdesc->numframes + 1))
		{
			pMonsterEvent->event = pevent[index].event;
			pMonsterEvent->options = pevent[index].options;
			return index + 1;
		}
	}
	return 0;
}

float SetController( void *pmodel, entvars_t *pev, int iController, float flValue )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (! pstudiohdr)
		return flValue;

	mstudiobonecontroller_t	*pbonecontroller = (mstudiobonecontroller_t *)((byte *)pstudiohdr + pstudiohdr->bonecontrollerindex);

	// find first controller that matches the index
	int i = 0;
	for (i = 0; i < pstudiohdr->numbonecontrollers; ++i, ++pbonecontroller)
	{
		if (pbonecontroller->index == iController)
			break;
	}
	if (i >= pstudiohdr->numbonecontrollers)
		return flValue;

	// wrap 0..360 if it's a rotational controller

	if (pbonecontroller->type & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pbonecontroller->end < pbonecontroller->start)
			flValue = -flValue;

		// does the controller not wrap?
		if (pbonecontroller->start + 359.0f >= pbonecontroller->end)
		{
			if (flValue > ((pbonecontroller->start + pbonecontroller->end) / 2.0f) + 180.0f)
				flValue -= 360;
			if (flValue < ((pbonecontroller->start + pbonecontroller->end) / 2.0f) - 180.0f)
				flValue += 360;
		}
		else
		{
			if (flValue > 360.0f)
				flValue -= (int)(flValue / 360.0f) * 360.0f;
			else if (flValue < 0.0f)
				flValue += (int)((flValue / -360.0f) + 1.0f) * 360.0f;
		}
	}

	int setting = 255.0f * (flValue - pbonecontroller->start) / (pbonecontroller->end - pbonecontroller->start);
	if (setting < 0) setting = 0;
	else if (setting > 255) setting = 255;
	pev->controller[iController] = setting;

	return setting * (1.0f / 255.0f) * (pbonecontroller->end - pbonecontroller->start) + pbonecontroller->start;
}


float SetBlending(void *pmodel, entvars_t *pev, byte iBlender, float flValue)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return flValue;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex) + (int)pev->sequence;

	if (pseqdesc->blendtype[iBlender] == 0)
		return flValue;

	if (pseqdesc->blendtype[iBlender] & (STUDIO_XR | STUDIO_YR | STUDIO_ZR))
	{
		// ugly hack, invert value if end < start
		if (pseqdesc->blendend[iBlender] < pseqdesc->blendstart[iBlender])
			flValue = -flValue;

		// does the controller not wrap?
		if (pseqdesc->blendstart[iBlender] + 359.0 >= pseqdesc->blendend[iBlender])
		{
			if (flValue > ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.0f) + 180.0f)
				flValue -= 360.0f;
			else if (flValue < ((pseqdesc->blendstart[iBlender] + pseqdesc->blendend[iBlender]) / 2.0f) - 180.0f)
				flValue += 360.0f;
		}
	}

	short setting = 255.0f * (flValue - pseqdesc->blendstart[iBlender]) / (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]);
	if (setting < 0) setting = 0;
	else if (setting > 255) setting = 255;

	pev->blending[iBlender] = setting;
	return setting * (1.0f / 255.0f) * (pseqdesc->blendend[iBlender] - pseqdesc->blendstart[iBlender]) + pseqdesc->blendstart[iBlender];
}

int FindTransition( void *pmodel, int iEndingAnim, int iGoalAnim, int *piDir )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
		return iGoalAnim;

	mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

	// bail if we're going to or from a node 0
	if (pseqdesc[iEndingAnim].entrynode == 0 || pseqdesc[iGoalAnim].entrynode == 0)
		return iGoalAnim;

	int	iEndNode;

	//conprintf(0, "from %d to %d: ", pEndNode->iEndNode, pGoalNode->iStartNode);

	if (*piDir > 0)
		iEndNode = pseqdesc[iEndingAnim].exitnode;
	else
		iEndNode = pseqdesc[iEndingAnim].entrynode;

	if (iEndNode == pseqdesc[iGoalAnim].entrynode)
	{
		*piDir = 1;
		return iGoalAnim;
	}

	byte *pTransition = ((byte *)pstudiohdr + pstudiohdr->transitionindex);

	int iInternNode = pTransition[(iEndNode-1)*pstudiohdr->numtransitions + (pseqdesc[iGoalAnim].entrynode-1)];

	if (iInternNode == 0)
		return iGoalAnim;

	int i;

	// look for someone going
	for (i = 0; i < pstudiohdr->numseq; ++i)
	{
		if (pseqdesc[i].entrynode == iEndNode && pseqdesc[i].exitnode == iInternNode)
		{
			*piDir = 1;
			return i;
		}
		if (pseqdesc[i].nodeflags)
		{
			if (pseqdesc[i].exitnode == iEndNode && pseqdesc[i].entrynode == iInternNode)
			{
				*piDir = -1;
				return i;
			}
		}
	}
	conprintf(0, "Error in transition graph %s\n", pstudiohdr->name);
	return iGoalAnim;
}

void SetBodygroup(void *pmodel, entvars_t *pev, int iGroup, int iValue)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
	{
		conprintf(0, "ERROR: Bad studio header in SetBodygroup!\n");
		return;
	}

	if (iGroup > pstudiohdr->numbodyparts)
		return;

	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex) + iGroup;
	if (!pbodypart || iValue >= pbodypart->nummodels)
		return;

	int iCurrent = (pev->body / pbodypart->base) % pbodypart->nummodels;
	pev->body = (pev->body - (iCurrent * pbodypart->base) + (iValue * pbodypart->base));
}

int GetBodygroup(void *pmodel, entvars_t *pev, int iGroup)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
	{
		conprintf(0, "ERROR: Bad studio header in GetBodygroup!\n");
		return 0;
	}

	if (iGroup > pstudiohdr->numbodyparts)// XDM3038: TODO: UNDONE: TESTME! CHECK! what if == ?
		return 0;

	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex) + iGroup;
	if (!pbodypart || pbodypart->nummodels <= 1)
		return 0;

	//int iCurrent = (pev->body / pbodypart->base) % pbodypart->nummodels;
	return (pev->body / pbodypart->base) % pbodypart->nummodels;
}

// XDM3038
int GetBodyGroupsCount(void *pmodel)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
	{
		conprintf(0, "ERROR: Bad studio header in GetBodyCount!\n");
		return 0;
	}
	return pstudiohdr->numbodyparts;
}

// XDM
int GetBodyCount(void *pmodel, int bodygroup)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
	{
		conprintf(0, "ERROR: Bad studio header in GetBodyCount!\n");
		return 0;
	}
	mstudiobodyparts_t *pbodypart = (mstudiobodyparts_t *)((byte *)pstudiohdr + pstudiohdr->bodypartindex) + bodygroup;
	if (pbodypart == NULL)
		return 0;

	return pbodypart->nummodels;
}

int GetSequenceCount(void *pmodel)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
	{
		conprintf(0, "ERROR: Bad studio header in GetBodyCount!\n");
		return 0;
	}
	return pstudiohdr->numseq;
}

void SetBones(void *pmodel, float (*data)[3], int datasize)
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pmodel;
	if (pstudiohdr == NULL)
	{
		conprintf(0, "ERROR: Bad studio header in SetBones!\n");
		return;
	}

	mstudiobone_t *pbones = (mstudiobone_t *)((byte *)pstudiohdr + pstudiohdr->boneindex);
	int i;
	int limit = min(pstudiohdr->numbones, datasize);
	// go through the bones
	for(i = 0; i < limit; ++i)//, ++pbone) // XDM3038c: as in optimization guide
	{
		//for (j = 0; j < 3; ++j)
		//	pbone->value[j] = data[i][j];
		pbones[i].value[0] = data[i][0];
		pbones[i].value[1] = data[i][1];
		pbones[i].value[2] = data[i][2];
	}
}
