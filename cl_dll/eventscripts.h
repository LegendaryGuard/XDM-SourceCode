//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// eventscripts.h
#if !defined ( EVENTSCRIPTSH )
#define EVENTSCRIPTSH

//int EV_IndexFromTrace(struct pmtrace_s *pTrace);
bool EV_IsPlayer(int idx);
bool EV_IsLocal(int idx);

void EV_START(struct event_args_s *args);// XDM3038c
cl_entity_t *EV_GetViewEntity(int entindex);// XDM3037
void EV_GetGunPosition(struct event_args_s *args, float *pos, const float *origin);
void EV_GetDefaultShellInfo(event_args_t *args, const float *origin, const float *velocity, float *ShellVelocity, float *ShellOrigin, const float *forward, const float *right, const float *up, float forwardScale, float upScale, float rightScale);
void EV_EjectBrass(float *origin, float *velocity, const float &rotation, const int &model, const int &body, const int &soundtype);
void EV_MuzzleFlash(int entindex);

void EV_PrintParams(const char *name, event_args_t *args);// XDM: debug

float PlayTextureSound(char chTextureType, float *origin, bool breakable);
int EV_DamageDecal(int iBulletType, char chTextureType);
bool EV_ShouldDoSmoke(int iBulletType, char chTextureType);

#define EMIT_SOUND					(*gEngfuncs.pEventAPI->EV_PlaySound)
#define STOP_SOUND					(*gEngfuncs.pEventAPI->EV_StopSound)

/*inline void STOP_SOUND(int ent, int channel, const char *sample)
{
	EMIT_SOUND(ent, g_vecZero, channel, sample, 0.0f, 0.0f, SND_STOP, PITCH_NORM);
}*/

#endif // EVENTSCRIPTSH
