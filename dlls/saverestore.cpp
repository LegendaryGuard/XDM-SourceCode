#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "globals.h"

CGlobalState gGlobalState;


//-----------------------------------------------------------------------------
// Purpose: EXPORT Engine may ask for a save game title/comment
//			XHL gives highest priority to a localizeable title
// Input  : *pszBuffer - output buffer
//			iSizeBuffer - output buffer size
//-----------------------------------------------------------------------------
/*extern "C" void DLLEXPORT STDCALL SV_SaveGameComment(char *pszBuffer, int iSizeBuffer)
{
	//DBG_PRINTF("SV_SaveGameComment()\n");
	if (g_pWorld)
	{
		if (!FStringNull(g_pWorld->pev->netname))
			strncpy(pszBuffer, STRING(g_pWorld->pev->netname), iSizeBuffer-1);
		else if (!FStringNull(g_pWorld->pev->message))
			strncpy(pszBuffer, STRING(g_pWorld->pev->message), iSizeBuffer-1);
	}
	else if (gpGlobals)
		strncpy(pszBuffer, STRING(gpGlobals->mapname), iSizeBuffer-1);
	else
		strncpy(pszBuffer, "XHL", 4);

	pszBuffer[iSizeBuffer - 1] = 0;
}*/

TYPEDESCRIPTION	gEntvarsDescription[] = 
{
	DEFINE_ENTITY_FIELD( classname, FIELD_STRING ),
	DEFINE_ENTITY_GLOBAL_FIELD( globalname, FIELD_STRING ),

	DEFINE_ENTITY_FIELD( origin, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_FIELD( oldorigin, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_FIELD( velocity, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( basevelocity, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( clbasevelocity, FIELD_VECTOR ),// ?

	DEFINE_ENTITY_FIELD( movedir, FIELD_VECTOR ),

	DEFINE_ENTITY_FIELD( angles, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( avelocity, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( punchangle, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( v_angle, FIELD_VECTOR ),

	// For parametric entities
	DEFINE_ENTITY_FIELD( endpos, FIELD_POSITION_VECTOR ),// XDM3037: FIX?
	DEFINE_ENTITY_FIELD( startpos, FIELD_POSITION_VECTOR ),// XDM3037: FIX?
	DEFINE_ENTITY_FIELD( impacttime, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( starttime, FIELD_TIME ),

	DEFINE_ENTITY_FIELD( fixangle, FIELD_INTEGER ),// XDM3037: FIX!!!
	DEFINE_ENTITY_FIELD( idealpitch, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( pitch_speed, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( ideal_yaw, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( yaw_speed, FIELD_FLOAT ),

	DEFINE_ENTITY_FIELD( modelindex, FIELD_INTEGER ),
	DEFINE_ENTITY_GLOBAL_FIELD( model, FIELD_MODELNAME ),

	DEFINE_ENTITY_FIELD( viewmodel, FIELD_MODELNAME ),
	DEFINE_ENTITY_FIELD( weaponmodel, FIELD_MODELNAME ),

	DEFINE_ENTITY_FIELD( absmin, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_FIELD( absmax, FIELD_POSITION_VECTOR ),
	DEFINE_ENTITY_GLOBAL_FIELD( mins, FIELD_VECTOR ),
	DEFINE_ENTITY_GLOBAL_FIELD( maxs, FIELD_VECTOR ),
	DEFINE_ENTITY_GLOBAL_FIELD( size, FIELD_VECTOR ),

	DEFINE_ENTITY_FIELD( ltime, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( nextthink, FIELD_TIME ),

	DEFINE_ENTITY_FIELD( movetype, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( solid, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( skin, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( body, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( effects, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( gravity, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( friction, FIELD_FLOAT ),

	DEFINE_ENTITY_FIELD( light_level, FIELD_INTEGER ),// XDM3037: FIX!!!

	DEFINE_ENTITY_FIELD( sequence, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( gaitsequence, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( frame, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( animtime, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( framerate, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( controller, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( blending, FIELD_SHORT ),// XDM3037: FIX!!!

	DEFINE_ENTITY_FIELD( scale, FIELD_FLOAT ),

	DEFINE_ENTITY_FIELD( rendermode, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( renderamt, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( rendercolor, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( renderfx, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( health, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( frags, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( weapons, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( takedamage, FIELD_FLOAT ),

	DEFINE_ENTITY_FIELD( deadflag, FIELD_INTEGER ),// XDM3037: FIX!!!
	DEFINE_ENTITY_FIELD( view_ofs, FIELD_VECTOR ),

	DEFINE_ENTITY_FIELD( button, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( impulse, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( chain, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( dmg_inflictor, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( enemy, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( aiment, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( owner, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( groundentity, FIELD_EDICT ),

	DEFINE_ENTITY_FIELD( spawnflags, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( flags, FIELD_INTEGER ),// XDM3037: FIX!!!

	DEFINE_ENTITY_FIELD( colormap, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( team, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( max_health, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( teleport_time, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( armortype, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( armorvalue, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( waterlevel, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( watertype, FIELD_INTEGER ),
	// Having these fields be local to the individual levels makes it easier to test those levels individually.
	DEFINE_ENTITY_GLOBAL_FIELD( target, FIELD_STRING ),
	DEFINE_ENTITY_GLOBAL_FIELD( targetname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( netname, FIELD_STRING ),
	DEFINE_ENTITY_FIELD( message, FIELD_STRING ),

	DEFINE_ENTITY_FIELD( dmg_take, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( dmg_save, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( dmg, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( dmgtime, FIELD_TIME ),

	DEFINE_ENTITY_FIELD( noise, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise1, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise2, FIELD_SOUNDNAME ),
	DEFINE_ENTITY_FIELD( noise3, FIELD_SOUNDNAME ),

	DEFINE_ENTITY_FIELD( speed, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( air_finished, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( pain_finished, FIELD_TIME ),
	DEFINE_ENTITY_FIELD( radsuit_finished, FIELD_TIME ),

// NONONONO! It's the entity itself!	DEFINE_ENTITY_FIELD( pContainingEntity, FIELD_EDICT ),

	DEFINE_ENTITY_FIELD( playerclass, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( maxspeed, FIELD_FLOAT ),// XDM: all below - just an experiment

	DEFINE_ENTITY_FIELD( fov, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( weaponanim, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( pushmsec, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( bInDuck, FIELD_INTEGER ),// XDM3035c: I'd rather spam saves than get another ton of crashes
	DEFINE_ENTITY_FIELD( flTimeStepSound, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( flSwimTime, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( flDuckTime, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( iStepLeft, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( flFallVelocity, FIELD_FLOAT ),

	DEFINE_ENTITY_FIELD( gamestate, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( oldbuttons, FIELD_INTEGER ),

	DEFINE_ENTITY_FIELD( groupinfo, FIELD_INTEGER ),

	// For mods
	DEFINE_ENTITY_FIELD( iuser1, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( iuser2, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( iuser3, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( iuser4, FIELD_INTEGER ),
	DEFINE_ENTITY_FIELD( fuser1, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( fuser2, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( fuser3, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( fuser4, FIELD_FLOAT ),
	DEFINE_ENTITY_FIELD( vuser1, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( vuser2, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( vuser3, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( vuser4, FIELD_VECTOR ),
	DEFINE_ENTITY_FIELD( euser1, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( euser2, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( euser3, FIELD_EDICT ),
	DEFINE_ENTITY_FIELD( euser4, FIELD_EDICT )
};

#define ENTVARS_COUNT		(sizeof(gEntvarsDescription)/sizeof(gEntvarsDescription[0]))

void EntvarsKeyvalue(entvars_t *pev, KeyValueData *pkvd)
{
	size_t i;
	TYPEDESCRIPTION *pField;

	for (i = 0; i < ENTVARS_COUNT; ++i)
	{
		pField = &gEntvarsDescription[i];

		if (!_stricmp(pField->fieldName, pkvd->szKeyName))
		{
			switch( pField->fieldType )
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
				if (*pkvd->szValue == 0)
					conprintf(2, "Warning! Null string field in entity! %s:%s == \"%s\"\n", pkvd->szClassName, pkvd->szKeyName, pkvd->szValue);

			case FIELD_STRING:
				(*(int *)((char *)pev + pField->fieldOffset)) = ALLOC_STRING( pkvd->szValue );
				break;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*(float *)((char *)pev + pField->fieldOffset)) = atof( pkvd->szValue );
				break;

			case FIELD_INTEGER:
				(*(int *)((char *)pev + pField->fieldOffset)) = atoi( pkvd->szValue );
				break;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				{
					//UTIL_StringToVector( (float *)((char *)pev + pField->fieldOffset), pkvd->szValue );
					float *vec = (float *)((char *)pev + pField->fieldOffset);
					if (StringToVec(pkvd->szValue, vec) == false)
					{
						conprintf(0, "Bad vector field in entity! %s:%s == \"%s\"\n", pkvd->szClassName, pkvd->szKeyName, pkvd->szValue);
						VectorClear(vec);
					}
					//else if (strcmp(pkvd->szKeyName, "origin"))// XDM3038b: a little hack to avoid big problems? useless
					//	UTIL_SetOrigin(this, vec);
				}
				break;
			case FIELD_CLASSPTR:// XDM3038: accpets entindex
				{
					int ei = atoi(pkvd->szValue);
					if (ei == 0)
					{
						*((CBaseEntity **)((char *)pev + pField->fieldOffset)) = NULL;
					}
					else
					{
						CBaseEntity *pEntity = UTIL_EntityByIndex(ei);
						if (pEntity)
							*((CBaseEntity **)((char *)pev + pField->fieldOffset)) = pEntity;
						else
							conprintf(0, "Warning: %s: %s (pointer) is bad entity index!\n", pkvd->szClassName, pkvd->szKeyName);
					}
				}
				break;
			case FIELD_EDICT:// XDM3038: accpets entindex
				{
					int ei = atoi(pkvd->szValue);
					if (ei == 0)
					{
						*((edict_t **)((char *)pev + pField->fieldOffset)) = NULL;
					}
					else
					{
						edict_t *e = INDEXENT(ei);
						if (UTIL_IsValidEntity(e))
							*((edict_t **)((char *)pev + pField->fieldOffset)) = e;
						else
							conprintf(0, "Warning: %s: %s (edict) is bad entity index!\n", pkvd->szClassName, pkvd->szKeyName);
					}
				}
				break;
			case FIELD_EHANDLE:// XDM3038: accpets entindex
				{
					int ei = atoi(pkvd->szValue);
					if (ei == 0)
					{
						(*(EHANDLE *)((char *)pev + pField->fieldOffset)) = NULL;
					}
					else
					{
						CBaseEntity *pEntity = UTIL_EntityByIndex(ei);
						if (pEntity)
							(*(EHANDLE *)((char *)pev + pField->fieldOffset)) = pEntity;
						else
							conprintf(0, "Warning: %s: %s (EHANDLE) is bad entity index!\n", pkvd->szClassName, pkvd->szKeyName);
					}
				}
				break;
// unused			case FIELD_ENTITY:
// unused			case FIELD_EVARS:
// unused			case FIELD_POINTER:
			default:
				conprintf(0, "ERROR: Bad field in entity %s! %s (%d)\n", pkvd->szClassName, pkvd->szKeyName, pField->fieldType);
				break;
			}
			pkvd->fHandled = TRUE;
			return;
		}
	}
}

// --------------------------------------------------------------
//
// CSave
//
// --------------------------------------------------------------
static int gSizes[FIELD_TYPECOUNT] = 
{
	sizeof(float),		// FIELD_FLOAT
	sizeof(int),		// FIELD_STRING
	sizeof(int),		// FIELD_ENTITY
	sizeof(int),		// FIELD_CLASSPTR
	sizeof(int),		// FIELD_EHANDLE
	sizeof(int),		// FIELD_entvars_t
	sizeof(int),		// FIELD_EDICT
	sizeof(Vector),		// FIELD_VECTOR
	sizeof(Vector),		// FIELD_POSITION_VECTOR
	sizeof(int *),		// FIELD_POINTER
	sizeof(int),		// FIELD_INTEGER
#ifdef GNUC
	sizeof(int *)*2,	// FIELD_FUNCTION
#else
	sizeof(int *),		// FIELD_FUNCTION
#endif
	sizeof(int),		// FIELD_BOOLEAN
	sizeof(short),		// FIELD_SHORT
	sizeof(char),		// FIELD_CHARACTER
	sizeof(float),		// FIELD_TIME
	sizeof(int),		// FIELD_MODELNAME
	sizeof(int),		// FIELD_SOUNDNAME
	sizeof(uint8),		// FIELD_UINT8
	sizeof(uint16),		// FIELD_UINT16
	sizeof(uint32),		// FIELD_UINT32
};

// Base class includes common SAVERESTOREDATA pointer, and manages the entity table
CSaveRestoreBuffer::CSaveRestoreBuffer(void)
{
	m_pdata = NULL;
}

CSaveRestoreBuffer::CSaveRestoreBuffer(const CSaveRestoreBuffer &other)
{
	m_pdata = other.m_pdata;
}

CSaveRestoreBuffer::CSaveRestoreBuffer(SAVERESTOREDATA *pdata)
{
	m_pdata = pdata;
}

CSaveRestoreBuffer::~CSaveRestoreBuffer(void)
{
}

void CSaveRestoreBuffer::operator=(CSaveRestoreBuffer &other)
{
	m_pdata = other.m_pdata;
}

int	CSaveRestoreBuffer::EntityIndex(CBaseEntity *pEntity)
{
	if (pEntity == NULL)
		return -1;
	return EntityIndex(pEntity->pev);
}

int	CSaveRestoreBuffer::EntityIndex(entvars_t *pevLookup)
{
	if (pevLookup == NULL)
		return -1;
	return EntityIndex(ENT(pevLookup));
}

int	CSaveRestoreBuffer::EntityIndex(EOFFSET eoLookup)
{
	return EntityIndex(ENT(eoLookup));
}

int	CSaveRestoreBuffer::EntityIndex(edict_t *pentLookup)
{
	if (!m_pdata || pentLookup == NULL)
		return -1;

	int i;
	ENTITYTABLE *pTable;
	for (i = 0; i < m_pdata->tableCount; ++i)
	{
		pTable = m_pdata->pTable + i;
		if (pTable->pent == pentLookup)
			return i;
	}
	return -1;
}

edict_t *CSaveRestoreBuffer::EntityFromIndex( int entityIndex )
{
	if (!m_pdata || entityIndex < 0)
		return NULL;

	int i;
	ENTITYTABLE *pTable;
	for (i = 0; i < m_pdata->tableCount; ++i)
	{
		pTable = m_pdata->pTable + i;
		if (pTable->id == entityIndex)
			return pTable->pent;
	}
	return NULL;
}

int	CSaveRestoreBuffer::EntityFlagsSet(int entityIndex, int flags)
{
	if (!m_pdata || entityIndex < 0)
		return 0;
	if (entityIndex > m_pdata->tableCount)
		return 0;

	m_pdata->pTable[ entityIndex ].flags |= flags;

	return m_pdata->pTable[ entityIndex ].flags;
}

void CSaveRestoreBuffer::BufferRewind( int size )
{
	if ( !m_pdata )
		return;

	if ( m_pdata->size < size )
		size = m_pdata->size;

	m_pdata->pCurrentData -= size;
	m_pdata->size -= size;
}

#ifndef _WIN32
extern "C" {
unsigned _rotr ( unsigned val, int shift)
{
	register unsigned lobit;        /* non-zero means lo bit set */
	register unsigned num = val;    /* number to rotate */

	shift &= 0x1f;                  /* modulo 32 -- this will also make negative shifts work */

	while (shift--) {
		lobit = num & 1;        /* get high bit */
		num >>= 1;              /* shift right one bit */
		if (lobit)
			num |= 0x80000000;  /* set hi bit if lo bit was set */
	}
	return num;
}
}
#endif

unsigned int CSaveRestoreBuffer::HashString( const char *pszToken )
{
	unsigned int	hash = 0;
	while ( *pszToken )
		hash = _rotr( hash, 4 ) ^ *pszToken++;

	return hash;
}

unsigned short CSaveRestoreBuffer::TokenHash( const char *pszToken )
{
	unsigned short	hash = (unsigned short)(HashString( pszToken ) % (unsigned)m_pdata->tokenCount );
	
#if _DEBUG
	static int tokensparsed = 0;
	tokensparsed++;
	if ( !m_pdata->tokenCount || !m_pdata->pTokens )
		ALERT( at_error, "No token table array in TokenHash()!" );
#endif

	for (/*unsigned */int i=0; i<m_pdata->tokenCount; ++i)
	{
#if _DEBUG
		static qboolean beentheredonethat = FALSE;
		if ( i > 50 && !beentheredonethat )
		{
			beentheredonethat = TRUE;
			ALERT( at_error, "CSaveRestoreBuffer::TokenHash() is getting too full!" );
		}
#endif

		int	index = hash + i;
		if (index >= m_pdata->tokenCount)
			index -= m_pdata->tokenCount;

		if ( !m_pdata->pTokens[index] || strcmp( pszToken, m_pdata->pTokens[index] ) == 0 )
		{
			m_pdata->pTokens[index] = (char *)pszToken;
			return index;
		}
	}
		
	// Token hash table full!!! 
	// [Consider doing overflow table(s) after the main table & limiting linear hash table search]
	ALERT( at_error, "CSaveRestoreBuffer::TokenHash() is COMPLETELY FULL!" );
	return 0;
}

void CSave::WriteData( const char *pname, int size, const char *pdata )
{
	BufferField( pname, size, pdata );
}

void CSave::WriteShort( const char *pname, const short *data, int count )
{
	BufferField( pname, sizeof(short) * count, (const char *)data );
}

void CSave::WriteInt( const char *pname, const int *data, int count )
{
	BufferField( pname, sizeof(int) * count, (const char *)data );
}

void CSave::WriteFloat( const char *pname, const float *data, int count )
{
	BufferField( pname, sizeof(float) * count, (const char *)data );
}

void CSave::WriteTime( const char *pname, const float *data, int count )
{
	int i;
	float tmp;
	BufferHeader( pname, sizeof(float) * count );
	for (i = 0; i < count; ++i)
	{
		tmp = data[0];
		// Always encode time as a delta from the current time so it can be re-based if loaded in a new level
		// Times of 0 are never written to the file, so they will be restored as 0, not a relative time
		if ( m_pdata )
			tmp -= m_pdata->time;

		BufferData( (const char *)&tmp, sizeof(float) );
		data ++;
	}
}

void CSave::WriteString( const char *pname, const char *pdata )
{
#ifdef TOKENIZE
	short	token = (short)TokenHash( pdata );
	WriteShort( pname, &token, 1 );
#else
	BufferField( pname, strlen(pdata) + 1, pdata );
#endif
}

void CSave::WriteString( const char *pname, const int *stringId, int count )
{
	int i, size;

#ifdef TOKENIZE
	short	token = (short)TokenHash( STRING( *stringId ) );
	WriteShort( pname, &token, 1 );
#else
#if 0
	if ( count != 1 )
		ALERT( at_error, "No string arrays!\n" );
	WriteString( pname, STRINGV(*stringId) );
#endif

	size = 0;
	for ( i = 0; i < count; ++i)
		size += strlen( STRING( stringId[i] ) ) + 1;

	BufferHeader( pname, size );
	for ( i = 0; i < count; i++ )
	{
		const char *pString = STRING(stringId[i]);
		BufferData( pString, strlen(pString)+1 );
	}
#endif
}

void CSave::WriteVector( const char *pname, const Vector &value )
{
	WriteVector( pname, &value.x, 1 );
}

void CSave::WriteVector( const char *pname, const float *value, int count )
{
	BufferHeader( pname, sizeof(float) * 3 * count );
	BufferData( (const char *)value, sizeof(float) * 3 * count );
}

void CSave::WritePositionVector( const char *pname, const Vector &value )
{
	if ( m_pdata && m_pdata->fUseLandmark )
	{
		Vector tmp = value - m_pdata->vecLandmarkOffset;
		WriteVector( pname, tmp );
	}
	WriteVector( pname, value );
}

void CSave::WritePositionVector( const char *pname, const float *value, int count )
{
	int i;
	Vector tmp, input;

	BufferHeader( pname, sizeof(float) * 3 * count );
	for ( i = 0; i < count; ++i)
	{
		tmp.Set(value[0], value[1], value[2]);

		if ( m_pdata && m_pdata->fUseLandmark )
			tmp -= m_pdata->vecLandmarkOffset;

		BufferData( (const char *)&tmp.x, sizeof(float) * 3 );
		value += 3;
	}
}

void CSave::WriteFunction( const char *pname, void **data, int count )
{
	const char *functionName = NAME_FOR_FUNCTION( (uint32)*data );
	if (functionName)
		BufferField( pname, strlen(functionName) + 1, functionName );
	else
		conprintf(0, "ERROR: Invalid function pointer (%s) in entity!\n", pname);
}

int CSave::WriteEntVars(/* const char *pname, */entvars_t *pev )
{
	return WriteFields(ENTVARS_SECTION_NAME, pev, gEntvarsDescription, ENTVARS_COUNT);// XDM3037: it's a constant anyway
}

int CSave::WriteFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	int				i, j, actualCount, emptyCount;
	TYPEDESCRIPTION	*pTest;
	int				entityArray[MAX_ENTITYARRAY];
	void *pOutputData;

	// Precalculate the number of empty fields
	emptyCount = 0;
	for (i = 0; i < fieldCount; ++i)
	{
		pOutputData = ((char *)pBaseData + pFields[i].fieldOffset );
		if ( DataEmpty( (const char *)pOutputData, pFields[i].fieldSize * gSizes[pFields[i].fieldType] ) )
			emptyCount++;
	}

	// Empty fields will not be written, write out the actual number of fields to be written
	actualCount = fieldCount - emptyCount;
	WriteInt(pname, &actualCount, 1);

	for ( i = 0; i < fieldCount; i++ )
	{
		pTest = &pFields[ i ];
		pOutputData = ((char *)pBaseData + pTest->fieldOffset);

		// UNDONE: Must we do this twice?
		if ( DataEmpty( (const char *)pOutputData, pTest->fieldSize * gSizes[pTest->fieldType] ) )
			continue;

		switch( pTest->fieldType )
		{
		case FIELD_FLOAT:
			WriteFloat( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
		break;
		case FIELD_TIME:
			WriteTime( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
		break;
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			WriteString( pTest->fieldName, (int *)pOutputData, pTest->fieldSize );
		break;
		case FIELD_CLASSPTR:
		case FIELD_EVARS:
		case FIELD_EDICT:
		case FIELD_ENTITY:
		case FIELD_EHANDLE:
			if ( pTest->fieldSize > MAX_ENTITYARRAY )
				ALERT( at_error, "Can't save more than %d entities in an array!!!\n", MAX_ENTITYARRAY );
			for ( j = 0; j < pTest->fieldSize; j++ )
			{
				switch( pTest->fieldType )
				{
					case FIELD_EVARS:
						entityArray[j] = EntityIndex( ((entvars_t **)pOutputData)[j] );
					break;
					case FIELD_CLASSPTR:
						entityArray[j] = EntityIndex( ((CBaseEntity **)pOutputData)[j] );
					break;
					case FIELD_EDICT:
						entityArray[j] = EntityIndex( ((edict_t **)pOutputData)[j] );
					break;
					case FIELD_ENTITY:
						entityArray[j] = EntityIndex( ((EOFFSET *)pOutputData)[j] );
					break;
					case FIELD_EHANDLE:
						entityArray[j] = EntityIndex( (CBaseEntity *)(((EHANDLE *)pOutputData)[j]) );

					//conprintf(2, "WriteFields(FIELD_EHANDLE, %s set to ent %d\n", pTest->fieldName, entityArray[j]);// XDM3038: DEBUG
					break;
				}
			}
			WriteInt( pTest->fieldName, entityArray, pTest->fieldSize );
		break;
		case FIELD_POSITION_VECTOR:
			WritePositionVector( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
		break;
		case FIELD_VECTOR:
			WriteVector( pTest->fieldName, (float *)pOutputData, pTest->fieldSize );
		break;

		case FIELD_BOOLEAN:
		case FIELD_INTEGER:
			WriteInt( pTest->fieldName, (int *)pOutputData, pTest->fieldSize );
		break;

		case FIELD_SHORT:
			WriteData(pTest->fieldName, sizeof(short)*pTest->fieldSize, ((char *)pOutputData));// XDM3038c: sizeof instead of 2
		break;

		case FIELD_CHARACTER:
			WriteData(pTest->fieldName, pTest->fieldSize, ((char *)pOutputData));
		break;

		// For now, just write the address out, we're not going to change memory while doing this yet!
		case FIELD_POINTER:
			WriteInt( pTest->fieldName, (int *)(char *)pOutputData, pTest->fieldSize );
		break;

		case FIELD_FUNCTION:
			WriteFunction( pTest->fieldName, (void **)pOutputData, pTest->fieldSize );
		break;

		case FIELD_UINT8:
			WriteData(pTest->fieldName, sizeof(uint8)*pTest->fieldSize, ((char *)pOutputData));
		break;
		case FIELD_UINT16:
			WriteData(pTest->fieldName, sizeof(uint16)*pTest->fieldSize, ((char *)pOutputData));
		break;
		case FIELD_UINT32:
			WriteData(pTest->fieldName, sizeof(uint32)*pTest->fieldSize, ((char *)pOutputData));
		break;

		default:
			ALERT(at_error, "Bad field type: %s %d offset %d\n", pTest->fieldName, pTest->fieldType, pTest->fieldOffset);
		}
	}

	return 1;
}

void CSave::BufferString( char *pdata, int len )
{
	char c = 0;

	BufferData( pdata, len );		// Write the string
	BufferData( &c, 1 );			// Write a null terminator
}

int CSave::DataEmpty( const char *pdata, int size )
{
	for ( int i = 0; i < size; i++ )
	{
		if ( pdata[i] )
			return 0;
	}
	return 1;
}

void CSave::BufferField( const char *pname, int size, const char *pdata )
{
	BufferHeader( pname, size );
	BufferData( pdata, size );
}

void CSave::BufferHeader( const char *pname, int size )
{
	short	hashvalue = TokenHash( pname );
	if ( size > 1<<(sizeof(short)*8) )
		ALERT( at_error, "CSave::BufferHeader() size parameter exceeds 'short'!" );

	BufferData( (const char *)&size, sizeof(short) );
	BufferData( (const char *)&hashvalue, sizeof(short) );
}

void CSave::BufferData( const char *pdata, int size )
{
	if ( !m_pdata )
		return;

	if (m_pdata->size + size > m_pdata->bufferSize)
	{
		ALERT(at_error, "CSave::BufferData() overflow!\n");
		m_pdata->size = m_pdata->bufferSize;
		return;
	}

	memcpy( m_pdata->pCurrentData, pdata, size );
	m_pdata->pCurrentData += size;
	m_pdata->size += size;
}


// --------------------------------------------------------------
//
// CRestore
//
// --------------------------------------------------------------
#include <vector>

int CRestore::ReadField( void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount, int startField, int size, char *pName, void *pData )
{
	int i, j, stringCount, fieldNumber, entityIndex;
	TYPEDESCRIPTION *pTest;
	float	time, timeData;
	Vector	position;
	edict_t	*pent;
	char	*pString;
	//short	iFieldSize;

	time = 0;
	position.Clear();

	if (m_pdata)
	{
		time = m_pdata->time;
		if (m_pdata->fUseLandmark)
			position = m_pdata->vecLandmarkOffset;
	}

	for (i = 0; i < fieldCount; ++i)
	{
		fieldNumber = (i+startField)%fieldCount;
		pTest = &pFields[ fieldNumber ];
		if (!_stricmp(pTest->fieldName, pName))
		{
			if (!m_global || !FBitSet(pTest->flags, FTYPEDESC_GLOBAL))
			{
				/*if (FBitSet(pTest->flags, FTYPEDESC_STD_VECTOR))// XDM3038c
				{
					void *pInputData = (char *)pData + j * gSizes[pTest->fieldType];
					HOW?!
					iFieldSize = ((std::vector<EHANDLE>)pInputData).size();
					iFieldSize = ((std::vector *)pInputData).size();
				}
				else
					iFieldSize = pTest->fieldSize;*/

				for (j = 0; j < pTest->fieldSize; ++j)
				{
					void *pOutputData = ((char *)pBaseData + pTest->fieldOffset + (j*gSizes[pTest->fieldType]));
					void *pInputData = (char *)pData + j * gSizes[pTest->fieldType];

					switch (pTest->fieldType)
					{
					case FIELD_TIME:
						timeData = *(float *)pInputData;
						// Re-base time variables
						timeData += time;
						*((float *)pOutputData) = timeData;
					break;
					case FIELD_FLOAT:
						*((float *)pOutputData) = *(float *)pInputData;
					break;
					case FIELD_MODELNAME:
					case FIELD_SOUNDNAME:
					case FIELD_STRING:
						// Skip over j strings
						pString = (char *)pData;
						for (stringCount = 0; stringCount < j; ++stringCount)
						{
							while (*pString)
								++pString;
							pString++;
						}
						pInputData = pString;
						if (strlen((char *)pInputData) == 0)
							*((int *)pOutputData) = 0;
						else
						{
							int string = ALLOC_STRING((char *)pInputData);
							*((int *)pOutputData) = string;
							if (!FStringNull(string) && m_precache)
							{
								if (pTest->fieldType == FIELD_MODELNAME )
									PRECACHE_MODEL(STRINGV(string));
								else if (pTest->fieldType == FIELD_SOUNDNAME )
									PRECACHE_SOUND(STRINGV(string));
							}
						}
					break;
					case FIELD_EVARS:
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((entvars_t **)pOutputData) = VARS(pent);
						else
							*((entvars_t **)pOutputData) = NULL;
					break;
					case FIELD_CLASSPTR:
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((CBaseEntity **)pOutputData) = CBaseEntity::Instance(pent);
						else
							*((CBaseEntity **)pOutputData) = NULL;
					break;
					case FIELD_EDICT:
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex(entityIndex);
						*((edict_t **)pOutputData) = pent;
					break;
					case FIELD_EHANDLE:
						// Input and Output sizes are different!
						pOutputData = (char *)pOutputData + j*(sizeof(EHANDLE) - gSizes[pTest->fieldType]);
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex(entityIndex);
						//conprintf(2, "ReadField(FIELD_EHANDLE, %s set to ent %d (%s)\n", pTest->fieldName, entityIndex, pent?STRING(pent->v.classname):"NULL");// XDM3038: DEBUG
						if (pent)
							*((EHANDLE *)pOutputData) = CBaseEntity::Instance(pent);
						else
							*((EHANDLE *)pOutputData) = NULL;
					break;
					case FIELD_ENTITY:
						entityIndex = *(int *)pInputData;
						pent = EntityFromIndex(entityIndex);
						if (pent)
							*((EOFFSET *)pOutputData) = OFFSET(pent);
						else
							*((EOFFSET *)pOutputData) = 0;
					break;
					case FIELD_VECTOR:
						((float *)pOutputData)[0] = ((float *)pInputData)[0];
						((float *)pOutputData)[1] = ((float *)pInputData)[1];
						((float *)pOutputData)[2] = ((float *)pInputData)[2];
					break;
					case FIELD_POSITION_VECTOR:
						((float *)pOutputData)[0] = ((float *)pInputData)[0] + position.x;
						((float *)pOutputData)[1] = ((float *)pInputData)[1] + position.y;
						((float *)pOutputData)[2] = ((float *)pInputData)[2] + position.z;
					break;

					case FIELD_BOOLEAN:
					case FIELD_INTEGER:
						*((int *)pOutputData) = *(int *)pInputData;
					break;

					case FIELD_SHORT:
						*((short *)pOutputData) = *(short *)pInputData;
					break;

					case FIELD_CHARACTER:
						*((char *)pOutputData) = *(char *)pInputData;
					break;

					case FIELD_UINT8:
						*((uint8 *)pOutputData) = *(uint8 *)pInputData;
					break;

					case FIELD_UINT16:
						*((uint8 *)pOutputData) = *(uint16 *)pInputData;
					break;

					case FIELD_UINT32:
						*((uint8 *)pOutputData) = *(uint32 *)pInputData;
					break;

					case FIELD_POINTER:
						*((int *)pOutputData) = *(int *)pInputData;
					break;
					case FIELD_FUNCTION:
						if (strlen((char *)pInputData) == 0)
							*((int *)pOutputData) = 0;
						else
							*((int *)pOutputData) = FUNCTION_FROM_NAME((char *)pInputData);
					break;

					default:
						conprintf(0, " ReadField() ERROR: Bad field type: %d!\n", pTest->fieldType);
						break;
					}
				}
			}
#if 0
			else
			{
				conprintf(0, "Skipping global field %s\n", pName);
			}
#endif
			return fieldNumber;
		}
	}
	return -1;
}

int CRestore::ReadEntVars(/*const char *pname, */entvars_t *pev)
{
	return ReadFields(/*pname*/ENTVARS_SECTION_NAME, pev, gEntvarsDescription, ENTVARS_COUNT);
}

// Output : int - 1 on success, 0 on failure.
int CRestore::ReadFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	unsigned short	i, token;
	int		lastField, fileCount;
	HEADER	header;

	i = ReadShort();
	ASSERT( i == sizeof(int) );			// First entry should be an int

	token = ReadShort();

	// Check the struct name
	if ( token != TokenHash(pname) )			// Field Set marker
	{
		//ALERT( at_error,  "Expected %s found %s!\n", pname, BufferPointer() );
		BufferRewind( 2*sizeof(short) );
		return 0;
	}

	// Skip over the struct name
	fileCount = ReadInt();						// Read field count
	lastField = 0;								// Make searches faster, most data is read/written in the same order

	// Clear out base data
	for (i = 0; i < fieldCount; ++i)
	{
		// Don't clear global fields
		if (!m_global || !FBitSet(pFields[i].flags, FTYPEDESC_GLOBAL))
			memset( ((char *)pBaseData + pFields[i].fieldOffset), 0, pFields[i].fieldSize * gSizes[pFields[i].fieldType] );
	}

	for (i = 0; i < fileCount; ++i)
	{
		BufferReadHeader( &header );
		lastField = ReadField( pBaseData, pFields, fieldCount, lastField, header.size, m_pdata->pTokens[header.token], header.pData );
		++lastField;
	}

	return 1;
}

void CRestore::BufferReadHeader( HEADER *pheader )
{
	ASSERT( pheader!=NULL );
	if (pheader)
	{
		pheader->size = ReadShort();				// Read field size
		pheader->token = ReadShort();				// Read field name token
		pheader->pData = BufferPointer();			// Field Data is next
		BufferSkipBytes( pheader->size );			// Advance to next field
	}
}

short CRestore::ReadShort(void)
{
	short tmp = 0;
	BufferReadBytes( (char *)&tmp, sizeof(short) );
	return tmp;
}

int	CRestore::ReadInt(void)
{
	int tmp = 0;
	BufferReadBytes( (char *)&tmp, sizeof(int) );
	return tmp;
}

int CRestore::ReadNamedInt( const char *pName )
{
	HEADER header;
	BufferReadHeader( &header );
	return ((int *)header.pData)[0];
}

char *CRestore::ReadNamedString( const char *pName )
{
	HEADER header;
	BufferReadHeader( &header );
#ifdef TOKENIZE
	return (char *)(m_pdata->pTokens[*(short *)header.pData]);
#else
	return (char *)header.pData;
#endif
}

char *CRestore::BufferPointer(void)
{
	if ( !m_pdata )
		return NULL;

	return m_pdata->pCurrentData;
}

void CRestore::BufferReadBytes( char *pOutput, int size )
{
	ASSERT( m_pdata !=NULL );

	if ( !m_pdata || Empty() )
		return;

	if ( (m_pdata->size + size) > m_pdata->bufferSize )
	{
		ALERT( at_error, "Restore overflow!" );
		m_pdata->size = m_pdata->bufferSize;
		return;
	}

	if ( pOutput )
		memcpy( pOutput, m_pdata->pCurrentData, size );
	m_pdata->pCurrentData += size;
	m_pdata->size += size;
}

void CRestore::BufferSkipBytes( int bytes )
{
	BufferReadBytes( NULL, bytes );
}

int CRestore::BufferSkipZString(void)
{
	char *pszSearch;
	int	 len;

	if ( !m_pdata )
		return 0;

	int maxLen = m_pdata->bufferSize - m_pdata->size;

	len = 0;
	pszSearch = m_pdata->pCurrentData;
	while ( *pszSearch++ && len < maxLen )
		++len;

	++len;
	BufferSkipBytes( len );
	return len;
}

/* unused int	CRestore::BufferCheckZString( const char *string )
{
	if ( !m_pdata )
		return 0;

	int maxLen = m_pdata->bufferSize - m_pdata->size;
	int len = strlen(string);
	if (len <= maxLen)
	{
		if (!strncmp(string, m_pdata->pCurrentData, len))
			return 1;
	}
	return 0;
}*/



static char *estates[] = { "Off", "On", "Dead" };

// Global Savedata for Delay
TYPEDESCRIPTION	CGlobalState::m_SaveData[] =
{
	DEFINE_FIELD(CGlobalState, m_listCount, FIELD_INTEGER),
};

// Global Savedata for Delay
TYPEDESCRIPTION	gGlobalEntitySaveData[] =
{
	DEFINE_ARRAY(globalentity_t, name, FIELD_CHARACTER, 64),
	DEFINE_ARRAY(globalentity_t, levelName, FIELD_CHARACTER, MAX_MAPNAME),
	DEFINE_FIELD(globalentity_t, state, FIELD_INTEGER),
};

CGlobalState::CGlobalState(void)
{
	Reset();
}

void CGlobalState::Reset(void)
{
	m_pList = NULL;
	m_listCount = 0;
}

globalentity_t *CGlobalState::Find(string_t globalname)
{
	if (FStringNull(globalname))
		return NULL;

	const char *pEntityName = STRING(globalname);
	globalentity_t *pTest = m_pList;
	while (pTest)
	{
		if (FStrEq(pEntityName, pTest->name))
			break;

		pTest = pTest->pNext;
	}
	return pTest;
}

// This is available all the time now on impulse 104, remove later
//#if defined (_DEBUG)
void CGlobalState::DumpGlobals(void)
{
	conprintf(1, "-- Globals --\n");
	globalentity_t *pTest = m_pList;
	while (pTest)
	{
		conprintf(1, "%s: %s (%s)\n", pTest->name, pTest->levelName, estates[pTest->state]);
		pTest = pTest->pNext;
	}
}
//#endif

void CGlobalState::EntityAdd(string_t globalname, string_t mapName, GLOBALESTATE state)
{
	ASSERT(!Find(globalname));
	globalentity_t *pNewEntity = (globalentity_t *)calloc(sizeof(globalentity_t), 1);
	ASSERT(pNewEntity != NULL);
	if (pNewEntity)
	{
		conprintf(2, "CGlobalState::EntityAdd(\"%s\", \"%s\", %s)\n", STRING(globalname), STRING(mapName), estates[state]);
		pNewEntity->pNext = m_pList;
		m_pList = pNewEntity;
		strcpy(pNewEntity->name, STRING(globalname));
		strcpy(pNewEntity->levelName, STRING(mapName));
		pNewEntity->state = state;
		m_listCount++;
	}
}

void CGlobalState::EntitySetState(string_t globalname, GLOBALESTATE state)
{
	globalentity_t *pEnt = Find(globalname);
	if (pEnt)
		pEnt->state = state;
}

const globalentity_t *CGlobalState::EntityFromTable(string_t globalname)
{
	return Find(globalname);
}

GLOBALESTATE CGlobalState::EntityGetState(string_t globalname)
{
	globalentity_t *pEnt = Find(globalname);
	if (pEnt)
		return pEnt->state;

	return GLOBAL_OFF;
}

int CGlobalState::Save(CSave &save)
{
	if (!save.WriteFields("GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData)))
		return 0;

	globalentity_t *pEntity = m_pList;
	int i;
	for (i = 0; i < m_listCount && pEntity; ++i)
	{
		if (!save.WriteFields("GENT", pEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData)))
			return 0;

		pEntity = pEntity->pNext;
	}
	return 1;
}

int CGlobalState::Restore(CRestore &restore)
{
	int i, listCount;
	globalentity_t tmpEntity;

	ClearStates();
	if (!restore.ReadFields("GLOBAL", this, m_SaveData, ARRAYSIZE(m_SaveData)))
		return 0;

	listCount = m_listCount;	// Get new list count
	m_listCount = 0;				// Clear loaded data

	for (i = 0; i < listCount; ++i)
	{
		if (!restore.ReadFields("GENT", &tmpEntity, gGlobalEntitySaveData, ARRAYSIZE(gGlobalEntitySaveData)))
			return 0;
		EntityAdd(MAKE_STRING(tmpEntity.name), MAKE_STRING(tmpEntity.levelName), tmpEntity.state);
	}
	return 1;
}

void CGlobalState::EntityUpdate(string_t globalname, string_t mapname)
{
	globalentity_t *pEnt = Find(globalname);
	if (pEnt)
		strcpy(pEnt->levelName, STRING(mapname));
}

void CGlobalState::ClearStates(void)
{
	globalentity_t *pFree = m_pList;
	while (pFree)
	{
		globalentity_t *pNext = pFree->pNext;
		free(pFree);
		pFree = pNext;
	}
	Reset();
}


// XDM3038a: moved here
void SaveGlobalState(SAVERESTOREDATA *pSaveData)
{
	CSave saveHelper(pSaveData);
	gGlobalState.Save(saveHelper);
}

void RestoreGlobalState(SAVERESTOREDATA *pSaveData)
{
	CRestore restoreHelper(pSaveData);
	gGlobalState.Restore(restoreHelper);
}

void ResetGlobalState(void)
{
	gGlobalState.ClearStates();

	for (size_t i=0; i<=MAX_CLIENTS; ++i)//gpGlobals->maxClients; ++i)// update entire array
		g_ClientShouldInitialize[i] = 1;
	//gInitHUD = TRUE;	// Init the HUD on a new game / load game
}




//-----------------------------------------------------------------------------
// UNDONE: DO NOT USE THIS CODE! It's only a sketch!
// TODO: To be used by CoOp game rules to save data across transitions
//-----------------------------------------------------------------------------

bool SaveRestore_Seek(SAVERESTOREDATA *pSaveData, int absPosition)
{
	if (absPosition < 0 || absPosition >= pSaveData->bufferSize)
		return false;
	
	pSaveData->size = absPosition;
	pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;
	return true;
}

/*void SaveRestore_Init(SAVERESTOREDATA *pSaveData, void *pNewBase, int nBytes)
{
	pSaveData->pCurrentData = pSaveData->pBaseData = (char *)pNewBase;
	pSaveData->bufferSize = nBytes;
	pSaveData->size = 0;
}

void SaveRestore_InitEntityTable(SAVERESTOREDATA *pSaveData, ENTITYTABLE *pNewTable, int entityCount)
{
	ENTITYTABLE	*pTable;
	ASSERT(pSaveData->pTable == NULL);
	pSaveData->tableCount = entityCount;
	pSaveData->pTable = pNewTable;
	for (int i = 0; i < entityCount; ++i)
	{
		pTable = &pSaveData->pTable[i];		
		pTable->pent = INDEXENT(i);
	}
}*/

/*void SaveRestore_InitSymbolTable(SAVERESTOREDATA *pSaveData, char **pNewTokens, int sizeTable)
{
	ASSERT(pSaveData->pTokens == NULL);
	pSaveData->tokenCount = sizeTable;
	pSaveData->pTokens = pNewTokens;
}*/

SAVERESTOREDATA *SV_SaveInit(int size)
{
	conprintf(1, "GAME DLL: SV_SaveInit(%d)\n", size);
	SAVERESTOREDATA *pSaveData;
	const int nTokens = 0xFFF;// Assume a maximum of 4K-1 symbol table entries(each of some length)
	if (size <= 0)
		size = 0x200000;// Reserve 2Mb for now

	pSaveData = (SAVERESTOREDATA *)calloc(1, sizeof(SAVERESTOREDATA) + (sizeof(ENTITYTABLE) * gpGlobals->maxEntities) + size);//Mem_Alloc(host.mempool, sizeof(SAVERESTOREDATA) + (sizeof(ENTITYTABLE) * numents) + size);
	if (pSaveData)
	{
		//SaveRestore_Init(pSaveData, (char *)(pSaveData + 1), size); // skip the save structure
		pSaveData->pCurrentData = pSaveData->pBaseData = (char *)(pSaveData + 1);
		pSaveData->size = 0;
		pSaveData->bufferSize = size;
		pSaveData->tokenSize = 0;
		pSaveData->tokenCount = 0xFFF;
		pSaveData->pTokens = (char **)calloc(1, nTokens * sizeof(char *));
		if (pSaveData->pTokens)
		{
			//SaveRestore_InitSymbolTable(pSaveData, (char **)pNewTokens, nTokens);//SaveRestore_InitSymbolTable(pSaveData, (char **)Mem_Alloc(host.mempool, nTokens * sizeof(char*)), nTokens);
			pSaveData->tokenCount = nTokens;
			pSaveData->time = gpGlobals->time;
			pSaveData->vecLandmarkOffset.Clear();
			pSaveData->fUseLandmark = false;
			pSaveData->connectionCount = 0;
			//svgame.globals->pSaveData = pSaveData;
		}
		else
		{
			conprintf(1, "SV_SaveInit(%d) failed: error allocating pNewTokens!\n", size);
			free(pSaveData);
		}
		pSaveData->currentIndex = 0;
		pSaveData->tableCount = gpGlobals->maxEntities;
		pSaveData->connectionCount = 0;
		pSaveData->pTable = (ENTITYTABLE *)(pSaveData + 1);
		memset(pSaveData->levelList, 0, sizeof(LEVELLIST)*MAX_LEVEL_CONNECTIONS);
		pSaveData->fUseLandmark = 0;
		pSaveData->szLandmarkName[0] = '\0';
		//strncpy(pSaveData->szLandmarkName, 
		pSaveData->vecLandmarkOffset.Clear();
		pSaveData->time = gpGlobals->time;
		strncpy(pSaveData->szCurrentMapName, STRING(gpGlobals->mapname), 32);
		pSaveData->szCurrentMapName[21] = '\0';
		//gpGlobals->pSaveData = pSaveData;
	}
	else
	{
		conprintf(1, "SV_SaveInit(%d) failed: error allocating pSaveData!\n", size);
	}
	return pSaveData;
}

void SV_SaveFree(SAVERESTOREDATA *pSaveData)
{
	conprintf(1, "GAME DLL: SV_SaveFree()\n");
	if (pSaveData)
	{
		pSaveData->pBaseData = NULL;
		pSaveData->pCurrentData = NULL;
		if (pSaveData->pTokens)
		{
			free(pSaveData->pTokens);
			pSaveData->pTokens = NULL;
			pSaveData->tokenCount = 0;
		}
		if (pSaveData->pTable)
		{
			free(pSaveData->pTable);
			pSaveData->pTable = NULL;
		}
		free(pSaveData);
	}
}

SAVERESTOREDATA *SV_Save(void)
{
	SAVERESTOREDATA *pSaveData = SV_SaveInit(0);
	return pSaveData;
}

bool SV_SaveGameState(SAVERESTOREDATA *pSaveData)
{
	conprintf(1, "GAME DLL: SV_SaveGameState()\n");
	if (pSaveData == NULL)
		return false;
	//SaveFileSectionsInfo_t	sectionsInfo;
	//SaveFileSections_t		sections;
	//SAVERESTOREDATA *pSaveData = SV_SaveInit(0);
	// Save the data
	//sections.pData = pSaveData->pCurrentData;
	ASSERT(pSaveData->pTable == NULL);
	/*if (pSaveData->pTable != NULL)
	{
		conprintf(1, "SV_SaveGameState() failed: pSaveData->pTable != NULL!\n");
		return false;
	}*/
	if (pSaveData->tableCount != gpGlobals->maxEntities)
	{
		conprintf(1, "SV_SaveGameState() failed: tableCount (%d) != maxEntities (%d)!\n", pSaveData->tableCount, gpGlobals->maxEntities);
		return false;
	}
	ENTITYTABLE *pEntInfo;
	if (pSaveData->pTable != NULL)
	{
		conprintf(2, "SV_SaveGameState(): reusing existing ENTITYTABLE\n");
		pEntInfo = pSaveData->pTable;
	}
	else
	{
		conprintf(2, "SV_SaveGameState(): allocating ENTITYTABLE\n");
		pEntInfo = (ENTITYTABLE *)calloc(gpGlobals->maxEntities, sizeof(ENTITYTABLE));
	}
	//SaveRestore_InitEntityTable(pSaveData, pEntInfo, gpGlobals->maxEntities);
	int i;
	//int iUnusedClientSlots = MAX_PLAYERS - gpGlobals->maxClients;
	edict_t *pEdict = INDEXENT(0);
	//edict_t *pWriteEdict;
	if (pEdict && pSaveData->pTable)
	{
		for (i = 0; i < gpGlobals->maxEntities; i++)
		{
			pSaveData->pTable[i].id = i;
			pSaveData->pTable[i].flags = 0;
			pSaveData->pTable[i].location = 0;
			pSaveData->pTable[i].size = 0;
			//if (i == gpGlobals->maxClients)
			//	for (j = 0; j < iUnusedClientSlots; ++j)
			if (i > gpGlobals->maxClients && i <= MAX_CLIENTS)// i is a potential (unused) player slot
			{
				pSaveData->pTable[i].pent = NULL;
				pSaveData->pTable[i].flags |= FENTTABLE_PLAYER;
				pSaveData->pTable[i].classname = 0;
				// don't increment edict!
			}
			else
			{
				pSaveData->pTable[i].pent = pEdict;
				pSaveData->pTable[i].classname = pEdict->v.classname;
				++pEdict;// next entity
			}
		}
	}
	// Build the adjacent map list (after entity table build by game in presave)
	//pfnParmsChangeLevel();

	// write entity descriptions
	for (i = 0; i < gpGlobals->maxEntities; i++)
	{
		edict_t	*pent = INDEXENT(i);
		pEntInfo = &pSaveData->pTable[pSaveData->currentIndex];
		DispatchSave(pent, pSaveData);

		if (pent->v.flags & FL_CLIENT)// mark client
			pEntInfo->flags |= FENTTABLE_PLAYER;

		if (pEntInfo->classname && pEntInfo->size)
			pEntInfo->id = ENTINDEX(pent);

		pSaveData->currentIndex++; // move pointer
	}
	//pSaveData->szCurrentMapName
	//sectionsInfo.nBytesData = pSaveData->pCurrentData - sections.pData;
	return true;
}

bool SV_RestoreEntity(SAVERESTOREDATA *pSaveData, int entindex, edict_t *pent)
{
	conprintf(1, "GAME DLL: SV_RestoreEntity(%d)\n", entindex);
	if (pSaveData == NULL || pent == NULL)
		return false;

	ENTITYTABLE	*pEntInfo;
	// now spawn entities
	for (int i = 0; i < pSaveData->tableCount; i++)
	{
		pEntInfo = &pSaveData->pTable[i];
		if (pEntInfo->id == entindex)
		{
			if (pEntInfo->flags & FENTTABLE_PLAYER)
			{
				if (!(pent->v.flags & FL_CLIENT))
				{
					conprintf(1, "SV_RestoreEntity(%d) failed: tried to restore non-player entity from player table!\n", entindex);
					return false;
				}
			}
			else
			{
				if (pent->v.flags & FL_CLIENT)
				{
					conprintf(1, "SV_RestoreEntity(%d) failed: tried to restore player from non-player table!\n", entindex);
					return false;
				}
			}

			if (!FStrEq(pent->v.classname, pEntInfo->classname))
			{
				conprintf(1, "SV_RestoreEntity(%d) failed: tried to restore %s from %s table!\n", entindex, STRING(pent->v.classname), STRING(pEntInfo->classname));
				return false;
			}

			SaveRestore_Seek(pSaveData, pEntInfo->location);
			if (DispatchRestore(pent, pSaveData, false) < 0)
			{
				conprintf(1, "SV_RestoreEntity(%d) failed: DispatchRestore() rejected the entity.\n", entindex);
				return false;
			}
			else
				return true;
		}
	}
	conprintf(1, "SV_RestoreEntity(%d) failed: entindex not found in table!\n", entindex);
	return false;
}
