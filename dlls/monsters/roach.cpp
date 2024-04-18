#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "soundent.h"
#include "decals.h"
#include "colors.h"// XDM

enum roach_modes_e
{
	ROACH_IDLE = 0,
	ROACH_BORED,
	ROACH_SCARED_BY_ENT,
	ROACH_SCARED_BY_LIGHT,
	ROACH_SMELL_FOOD,
	ROACH_EAT
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
class CRoach : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void SetYawSpeed(void);
	virtual void EXPORT MonsterThink(void);
	virtual void Move(float flInterval);
	void PickNewDest(int iCondition);
	virtual void EXPORT Touch(CBaseEntity *pOther);
	virtual void Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib);
	virtual int Classify(void);
	virtual void Look(int iDistance);
	virtual int ISoundMask(void);
	virtual bool ShouldRespawn(void) const { return false; }// XDM3035b
	virtual void DeathSound(void);// XDM3038c

	// UNDONE: These don't necessarily need to be save/restored, but if we add more data, it may
	float m_flLastLightLevel;
	float m_flNextSmellTime;
	BOOL m_fLightHacked;
	int m_iMode;

	static const char *pDeathSounds[];// XDM3038c
};
LINK_ENTITY_TO_CLASS( monster_cockroach, CRoach );

const char *CRoach::pDeathSounds[] = 
{
	"roach/rch_die.wav",
	"roach/rch_smash.wav",
};

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CRoach::ISoundMask(void)
{
	return	bits_SOUND_CARCASS | bits_SOUND_MEAT;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CRoach::Classify(void)
{
	return m_iClass?m_iClass:CLASS_INSECT;// XDM
}

//=========================================================
// Touch
//=========================================================
void CRoach::Touch(CBaseEntity *pOther)
{
	if (pOther->pev->velocity.IsZero())// || !pOther->IsPlayer() || !FClassnameIs(ENT(pOther->pev), "monster_headcrab"))
		return;

	if (pOther->Classify() == CLASS_INSECT)// Classify())?
		return;

	// TODO: check if pOther is big or heavy!
	if (pOther->IsPushable() || pOther->IsPlayer() || (pOther->IsMonster() && IRelationship(pOther) == R_FR))
	{
		TraceResult	tr;
		UTIL_TraceLine(pev->origin + Vector(0,0,8), pev->origin - Vector(0,0,16), ignore_monsters, edict(), & tr);
		// This isn't really blood.  So you don't have to screen it out based on violence levels (UTIL_ShouldShowBlood())
		UTIL_BloodDecalTrace(&tr, m_bloodColor);// XDM
		Killed(pOther, pOther, GIB_NEVER);
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRoach::SetYawSpeed(void)
{
	pev->yaw_speed = 120;
}

//=========================================================
// Spawn
//=========================================================
void CRoach::Spawn(void)
{
	if (pev->health <= 0)
		pev->health = 1;
	if (m_bloodColor == 0)// XDM3038a: no custom value specified
		m_bloodColor = BLOOD_COLOR_CYAN;

	// test	m_iGibCount			= 0;
	CBaseMonster::Spawn();// XDM3038b: Precache();
	UTIL_SetSize(this, Vector(-1, -1, 0), Vector(1, 1, 1));
	pev->solid			= SOLID_TRIGGER;// XDM3035a: touch, but don't block!
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	pev->takedamage		= DAMAGE_YES;// not AIM
	pev->renderamt		= 1;// shell offset
	pev->rendercolor.Set(0, RANDOM_LONG(47, 200), RANDOM_LONG(47, 200));
	pev->renderfx		= kRenderFxGlowShell;
	pev->view_ofs.Set(0,0,2);// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	MonsterInit();
	SetActivity(ACT_IDLE);
	m_fLightHacked		= FALSE;
	m_flLastLightLevel	= -1;
	m_iMode				= ROACH_IDLE;
	m_flNextSmellTime	= gpGlobals->time;
	m_flDistLook		= 160.0f;// XDM3035a
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoach::Precache()
{
	if (FStringNull(pev->model))// XDM3037
		pev->model = MAKE_STRING("models/roach.mdl");
	if (m_szSoundDir[0] == '\0')// XDM3038c
		strcpy(m_szSoundDir, "roach");

	CBaseMonster::Precache();// XDM3038a
	PRECACHE_SOUND("roach/rch_walk.wav");
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}	

void CRoach::DeathSound(void)// XDM3038c
{
	EMIT_SOUND_ARRAY_DYN2(CHAN_VOICE, pDeathSounds, ATTN_IDLE, RANDOM_LONG(80,120));
}

//=========================================================
// Killed.
//=========================================================
void CRoach::Killed(CBaseEntity *pInflictor, CBaseEntity *pAttacker, int iGib)
{
	pev->solid = SOLID_NOT;
	DeathSound();// XDM3038c

	CSoundEnt::InsertSound(bits_SOUND_WORLD, pev->origin, 128, 1);

	if (!FNullEnt(pev->owner))// for monstermaker
	{
		CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
		if (pOwner)
			pOwner->DeathNotice(this);
	}
	Destroy();
}

//=========================================================
// MonsterThink, overridden for roaches.
//=========================================================
void CRoach::MonsterThink(void)
{
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		SetNextThink(1.0);
	else
		SetNextThink(0.1);// keep monster thinking

	float flInterval = StudioFrameAdvance( ); // animate

	if ( !m_fLightHacked )
	{
		// if light value hasn't been collection for the first time yet, 
		// suspend the creature for a second so the world finishes spawning, then we'll collect the light level.
		SetNextThink(1.0);
		m_fLightHacked = TRUE;
		return;
	}
	else if ( m_flLastLightLevel < 0 )
	{
		// collect light level for the first time, now that all of the lightmaps in the roach's area have been calculated.
		m_flLastLightLevel = Illumination();
	}

	switch ( m_iMode )
	{
	case ROACH_IDLE:
	case ROACH_EAT:
		{
			// if not moving, sample environment to see if anything scary is around. Do a radius search 'look' at random.
			if ( RANDOM_LONG(0,3) == 1 )
			{
				Look(m_flDistLook);
				if (HasConditions(bits_COND_SEE_FEAR))
				{
				// if see something scary
					pev->renderfx		= kRenderFxNone;// XDM3035a
					pev->rendercolor.x	= RANDOM_LONG(191, 255);// XDM
					pev->rendercolor.y	= RANDOM_LONG(191, 255);
					pev->rendercolor.z	= 0;
					//ALERT ( at_aiconsole, "Scared\n" );
					Eat( 30 +  ( RANDOM_LONG(0,14) ) );// roach will ignore food for 30 to 45 seconds
					PickNewDest( ROACH_SCARED_BY_ENT );
					SetActivity ( ACT_WALK );
				}
				else if ( RANDOM_LONG(0,149) == 1 )
				{
					// if roach doesn't see anything, there's still a chance that it will move. (boredom)
					pev->rendercolor.x	= 0;// XDM
					pev->rendercolor.y	= RANDOM_LONG(95, 255);
					pev->rendercolor.z	= RANDOM_LONG(95, 255);
					//ALERT ( at_aiconsole, "Bored\n" );
					PickNewDest( ROACH_BORED );
					SetActivity ( ACT_WALK );

					if ( m_iMode == ROACH_EAT )
					{
						EMIT_SOUND_DYN(edict(), CHAN_VOICE, "roach/rch_smash.wav", 0.75, ATTN_IDLE, 0, 70 + RANDOM_LONG(0,40));
						// roach will ignore food for 30 to 45 seconds if it got bored while eating. 
						Eat(30 + (RANDOM_LONG(0,14)));
					}
				}
			}
	
			// don't do this stuff if eating!
			if ( m_iMode == ROACH_IDLE )
			{
				pev->rendercolor.x	= RANDOM_LONG(0, 127);// XDM
				pev->rendercolor.y	= RANDOM_LONG(127, 255);
				pev->rendercolor.z	= RANDOM_LONG(127, 255);
				pev->renderfx		= kRenderFxGlowShell;// XDM3035a

				if ( FShouldEat() )
					Listen();

				if (Illumination() > m_flLastLightLevel)
				{
					// someone turned on lights!
					//ALERT ( at_console, "Lights!\n" );
					PickNewDest( ROACH_SCARED_BY_LIGHT );
					SetActivity ( ACT_WALK );
				}
				else if ( HasConditions(bits_COND_SMELL_FOOD) )
				{
					if (m_iAudibleList >= 0)
					{
					CSound *pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );
					// roach smells food and is just standing around. Go to food unless food isn't on same z-plane.
					if ( pSound && abs( pSound->m_vecOrigin.z - pev->origin.z ) <= 3 )
					{
						PickNewDest( ROACH_SMELL_FOOD );
						SetActivity ( ACT_WALK );
					}
					}
				}
			}

			break;
		}
	case ROACH_SCARED_BY_LIGHT:
		{
			// if roach was scared by light, then stop if we're over a spot at least as dark as where we started!
			if ( Illumination() <= m_flLastLightLevel )
			{
				SetActivity ( ACT_IDLE );
				m_flLastLightLevel = Illumination();// make this our new light level.
			}
			break;
		}
	}
	
	if ( m_flGroundSpeed != 0 )
	{
		Move( flInterval );
	}
}

//=========================================================
// Picks a new spot for roach to run to.(
//=========================================================
void CRoach :: PickNewDest ( int iCondition )
{
	Vector	vecNewDir;
	Vector	vecDest;
	vec_t	flDist;

	m_iMode = iCondition;

	if ( m_iMode == ROACH_SMELL_FOOD )
	{
		if (m_iAudibleList >= 0)
		{
			// find the food and go there.
			CSound *pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );
			if ( pSound )
			{
				m_Route[0].vecLocation.x = pSound->m_vecOrigin.x + ( 3 - RANDOM_LONG(0,5) );
				m_Route[0].vecLocation.y = pSound->m_vecOrigin.y + ( 3 - RANDOM_LONG(0,5) );
				m_Route[0].vecLocation.z = pSound->m_vecOrigin.z;
				m_Route[0].iType = bits_MF_TO_LOCATION;
				m_movementGoal = RouteClassify( m_Route[ 0 ].iType );
				return;
			}
		}
	}
	do 
	{
		// picks a random spot, requiring that it be at least 128 units away
		// else, the roach will pick a spot too close to itself and run in 
		// circles. this is a hack but buys me time to work on the real monsters.
		vecNewDir.x = RANDOM_FLOAT( -1, 1 );
		vecNewDir.y = RANDOM_FLOAT( -1, 1 );
		flDist = 256 + ( RANDOM_LONG(0,255) );
		vecDest = pev->origin + vecNewDir * flDist;

	} while ( ( vecDest - pev->origin ).Length2D() < 128 );

	m_Route[0].vecLocation.x = vecDest.x;
	m_Route[0].vecLocation.y = vecDest.y;
	m_Route[0].vecLocation.z = pev->origin.z;
	m_Route[0].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify( m_Route[ 0 ].iType );

	if ( RANDOM_LONG(0,9) == 1 )
	{
		// every once in a while, a roach will play a skitter sound when they decide to run
		EMIT_SOUND_DYN(edict(), CHAN_BODY, "roach/rch_walk.wav", GetSoundVolume(), ATTN_NORM, 0, 80 + RANDOM_LONG(0,39));
	}
}

//=========================================================
// roach's move function
//=========================================================
void CRoach::Move(float flInterval)
{
	float		flWaypointDist;
	Vector		vecApex;

	// local move to waypoint.
	flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();
	MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );

	ChangeYaw ( pev->yaw_speed );
	UTIL_MakeVectors( pev->angles );

	if ( RANDOM_LONG(0,7) == 1 )
	{
		// randomly check for blocked path.(more random load balancing)
		if (!WALK_MOVE(edict(), pev->ideal_yaw, 4, WALKMOVE_NORMAL))
		{
			// stuck, so just pick a new spot to run off to
			PickNewDest( m_iMode );
		}
	}

	WALK_MOVE(edict(), pev->ideal_yaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL);

	// if the waypoint is closer than step size, then stop after next step (ok for roach to overshoot)
	if ( flWaypointDist <= m_flGroundSpeed * flInterval )
	{
		// take truncated step and stop
		SetActivity ( ACT_IDLE );
		m_flLastLightLevel = Illumination();// this is roach's new comfortable light level

		if ( m_iMode == ROACH_SMELL_FOOD )
		{
			m_iMode = ROACH_EAT;
		}
		else
		{
			m_iMode = ROACH_IDLE;
		}
	}

	if ( RANDOM_LONG(0,149) == 1 && m_iMode != ROACH_SCARED_BY_LIGHT && m_iMode != ROACH_SMELL_FOOD )
	{
		// random skitter while moving as long as not on a b-line to get out of light or going to food
		PickNewDest( FALSE );
	}
}

//=========================================================
// Look - overriden for the roach, which can virtually see 
// 360 degrees.
//=========================================================
void CRoach::Look(int iDistance)
{
	// DON'T let visibility information from last frame sit around!
	ClearConditions( bits_COND_SEE_HATE |bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR );

	// don't let monsters outside of the player's PVS act up, or most of the interesting
	// things will happen before the player gets there!
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		return;

	int iSighted = 0;
	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, iDistance)) != NULL)
	{
		if (pEntity == this)
			continue;

		if (pEntity->Classify() == CLASS_GIB)// XDM
		{
			iSighted |= bits_COND_SMELL_FOOD;
			continue;
		}

		// only consider ents that can be damaged. !!!temporarily only considering other monsters and clients
		if (pEntity->IsPlayer() || pEntity->IsMonster())
		{
			/*FVisible(pEntity) &&*/
			if (pEntity->IsAlive())
			{
				// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
				// we see monsters other than the Enemy.
				switch (IRelationship(pEntity))
				{
				case R_FR:
					{
						if (!pEntity->pev->velocity.IsZero())// XDM3035a: only fear moving players
							iSighted |= bits_COND_SEE_FEAR;	
					}
					break;
				case R_HT:
				case R_NM:
					iSighted |= bits_COND_SMELL_FOOD;// XDM: insects 'HATE' gibs :)
					break;
				}
			}
			else if (pEntity->pev->velocity.IsZero())// "ooh, something died down here!"
			{
				iSighted |= bits_COND_SMELL_FOOD;
			}
		}
	}
	SetConditions( iSighted );
}
