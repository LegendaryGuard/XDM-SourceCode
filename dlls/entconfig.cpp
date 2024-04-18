#include "extdll.h"
#include "util.h"
#include "const.h"
#include "cbase.h"
#include "entconfig.h"
#include "game.h"
#include "gamerules.h"


bool g_MapConfigCommands = false;// hack
bool g_MultiplayerOnlyCommands = false;
int g_iGamerulesSpecificCommands = -1;
char g_iGamerulesSpecificCommandsOp[4];

int g_iNumEntitiesAddedExternally = 0;
const char g_MapPatchFileNameFormat[] = "maps/%s_patch.ent";// XDM3037: for both reading and writing!

// These should fit in standard stack space of 16384 bytes
#define MAX_ENTITY_KEYVALUES	136
#define MAX_ENT_KEYNAME			32
#define MAX_ENT_KEYVALUE		64

//-----------------------------------------------------------------------------
// Purpose: Parses an entity block and creates an entity. Same as in the engine.
// WARNING: does not check for duplicate entries or already existing entities!
//
// Input  : *pData - start of the buffer
// Output : char * - position where it stopped reading
//-----------------------------------------------------------------------------
char *SV_ParseEntity(char *pData)
{
	if (pData == NULL)
		return NULL;

	char *classname = NULL;
	char *pToken = NULL;
	size_t i = 0, num = 0;
	// we cannot use pointer to file data, COM_Parse doesn't allow us to, so just allocate some static buffers
	char names[MAX_ENTITY_KEYVALUES][MAX_ENT_KEYNAME];
	char values[MAX_ENTITY_KEYVALUES][MAX_ENT_KEYVALUE];
	//std::vector<KeyValueData> KeyValuePairs;
	KeyValueData kvd[MAX_ENTITY_KEYVALUES];

	for (num = 0; num < MAX_ENTITY_KEYVALUES; ++num)// TODO: make this code more reliable
	{	
		pData = COM_Parse(pData);// parse key
		pToken = COM_Token();
		if (pToken == NULL || *pToken == '}')// end (this is normal after last value was parsed)
			break;
		strncpy(names[num], pToken, MAX_ENT_KEYNAME);
		names[num][MAX_ENT_KEYNAME-1] = '\0';
		kvd[num].szKeyName = names[num];//pToken; temporary buffer

		pData = COM_Parse(pData);// parse value
		pToken = COM_Token();
		if (pToken == NULL || *pToken == '}')// end (this is not normal)
			break;
		strncpy(values[num], pToken, MAX_ENT_KEYVALUE);
		values[num][MAX_ENT_KEYVALUE-1] = '\0';
		kvd[num].szValue = values[num];//pToken;

		kvd[num].szClassName = NULL;// unknown
		kvd[num].fHandled = false;		

		if (classname == NULL)
		{
			if (strcmp(kvd[num].szKeyName, "classname") == 0)
				classname = kvd[num].szValue;
			else if (num == 0)// 1st key, but not a classname!
			{
				SERVER_PRINT("ERROR: ENT file: entity key/value pair list must start with classname!\n");
				return NULL;// stop parsing the file
			}
		}
	}

	if (num == 0)// no values
		return NULL;
	if (classname == NULL)// no classname - no entity
		return NULL;

	if (strcmp(classname, "worldspawn") == 0)// NEVER add a new world!
	{
		SERVER_PRINT("ENT file error: worldspawn detected!\n");
		//ALERT(at_error, "ENT file: worldspawn detected!\nThis is probably an exported ENT file.\nENT files in maps directory must contain only additional entities that should be added into the game!\n");
		return NULL;// looks like someone put a wrong FULL exported .ent file here
	}

	CBaseEntity *pEntity = CBaseEntity::Create(classname, g_vecZero, g_vecZero);// XDM3038c

	if (!UTIL_IsValidEntity(pEntity))//if (!UTIL_IsValidEntity(ent))//if (ent == NULL)// possible!
	{
		SERVER_PRINTF("SV_ParseEntity: unable to create entity: %s\n", classname);
		return NULL;
	}

	edict_t *ent = pEntity->edict();

	for (i = 0; i < num; ++i)
	{
		kvd[i].szClassName = classname;
		DispatchKeyValue(ent, &kvd[i]);
	}

	if (DispatchSpawn(ent) < 0)
	{
		REMOVE_ENTITY(ent);
		//ent = NULL;
	}
	else
	{
		g_iNumEntitiesAddedExternally++;
		char str[256];
		_snprintf(str, 256, "+added '%s'\n", classname);
		SERVER_PRINT(str);
	}
	return pData;
}

bool SV_WriteEdict(char *pData, edict_t *ent)
{
	if (pData && ent)
	{
		//_snprintf(pData, 32, "{");
		//SaveWriteFields(
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: read additional entities from map_patch.ent
//-----------------------------------------------------------------------------
void ENTCONFIG_Init(void)// START DEBUG IN SOFTWARE!!!
{
	char str[128];

	if (sv_loadentfile.value > 0.0f)
	{
		_snprintf(str, 127, g_MapPatchFileNameFormat, STRING(gpGlobals->mapname));
		int len = 0;
		g_iNumEntitiesAddedExternally = 0;
		char *pFileStart = (char *)LOAD_FILE_FOR_ME(str, &len);
		if (pFileStart)
		{
			SERVER_PRINT("Initializing external map entity list\n");
			char *pFileData = pFileStart;
			char *token;
			while ((pFileData = COM_Parse(pFileData)) != NULL)
			{
				token = COM_Token();
				if (token && token[0] == '{')// new entity block
				{
					pFileData = SV_ParseEntity(pFileData);// keep the pFileData pointer updated!
				}
			}
			FREE_FILE(pFileStart);
			pFileStart = NULL;
			_snprintf(str, 127, "Created %d additional entities\n", g_iNumEntitiesAddedExternally);
			SERVER_PRINT(str);
		}
	}
	else
		SERVER_PRINT("External map entity list loading is disasbled in the game DLL.\n");
}

//-----------------------------------------------------------------------------
// Purpose: execute map_pre.cfg before installing game rules
//-----------------------------------------------------------------------------
void ENTCONFIG_ExecMapPreConfig(void)
{
	char str[127];
	_snprintf(str, 127, "exec maps/%s_pre.cfg\n", STRING(gpGlobals->mapname));
	g_MapConfigCommands = true;
	SERVER_COMMAND(str);
	SERVER_EXECUTE();
	g_MapConfigCommands = false;
	g_MultiplayerOnlyCommands = false;
	g_iGamerulesSpecificCommands = -1;
	g_iGamerulesSpecificCommandsOp[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: execute map.cfg after installing game rules
//-----------------------------------------------------------------------------
void ENTCONFIG_ExecMapConfig(void)
{
	char str[127];
	_snprintf(str, 127, "exec maps/%s.cfg\n", STRING(gpGlobals->mapname));
	g_MapConfigCommands = true;
	SERVER_COMMAND(str);
	SERVER_EXECUTE();
	g_MapConfigCommands = false;
	g_MultiplayerOnlyCommands = false;
	g_iGamerulesSpecificCommands = -1;
	g_iGamerulesSpecificCommandsOp[0] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Write an entity to map config for future loading, VALUES MUST INCLUDE CLASSNAME AND ORIGIN!
// Input  : writekeynames - array of key names
//			writekeyvalues - array of key values
//			numkeys - number of KV items
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ENTCONFIG_WriteEntityVirtual(char *writekeynames[], char *writekeyvalues[], const size_t numkeys)
{
	if (writekeynames == NULL || writekeyvalues == NULL || numkeys == 0)
		return false;

	char mapentname[128];
	_snprintf(mapentname, 128, g_MapPatchFileNameFormat, STRING(gpGlobals->mapname));
	FILE *pMapConfig = LoadFile(mapentname, "a+");// Opens an empty file for both reading and writing. If the given file exists, its contents are destroyed.
	if (pMapConfig)
	{
		fseek(pMapConfig, 0L, SEEK_END);// APPEND!
		/* this is needed if ALL entities in the file were enclosed in {}
		fpos_t fpos;
		fpos_t offset = 1;
		  fgetpos(pMapConfig, &fpos);
		while (offset < 64)// search from the end
		{
			fseek(pMapConfig, -1, SEEK_CUR);
			fsetpos(pMapConfig, fpos-offset);
			if (fgetc(pMapConfig) == '}')
				break;
			++offset;
		}*/
		fprintf(pMapConfig, "{\n");

		// UNDONE: we don't have any GetKeyValue mechanism, so make the user do the stuff
		for (size_t i=0; i<numkeys; ++i)
			fprintf(pMapConfig, "\"%s\" \"%s\"\n", writekeynames[i], writekeyvalues[i]);

		// UNDONE: all entvars
		fprintf(pMapConfig, "}\n");
		fclose(pMapConfig);
	}
	else
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Write an entity to map config for future loading
// Input  : *pev - 
//			writekeynames - array of key names
//			writekeyvalues - array of key values
//			numkeys - number of KV items
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ENTCONFIG_WriteEntity(entvars_t *pev, char *writekeynames[], char *writekeyvalues[], const size_t numkeys)
{
	if (pev == NULL)
		return false;

	char mapentname[128];
	_snprintf(mapentname, 128, g_MapPatchFileNameFormat, STRING(gpGlobals->mapname));
	FILE *pMapConfig = LoadFile(mapentname, "a+");// Opens an empty file for both reading and writing. If the given file exists, its contents are destroyed.
	if (pMapConfig)
	{
		fseek(pMapConfig, 0L, SEEK_END);// APPEND!
		/* this is needed if ALL entities in the file were enclosed in {}
		fpos_t fpos;
		fpos_t offset = 1;
		  fgetpos(pMapConfig, &fpos);
		while (offset < 64)// search from the end
		{
			fseek(pMapConfig, -1, SEEK_CUR);
			fsetpos(pMapConfig, fpos-offset);
			if (fgetc(pMapConfig) == '}')
				break;
			++offset;
		}*/

		// XDM: wanted to use SaveWriteFields so much...
		//fprintf(pMapConfig, "{\n\"classname\" \"%s\"\n\"origin\" \"%g %g %g\"\n\"angles\" \"%g %g %g\"\n", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z, pev->angles.x, pev->angles.y, pev->angles.z);
		fprintf(pMapConfig, "{\n\"classname\" \"%s\"\n\"origin\" \"%g %g %g\"\n", STRING(pev->classname), pev->origin.x, pev->origin.y, pev->origin.z);

		if (!pev->angles.IsZero() && !pev->angles.IsEqualTo(360,0,0))
			fprintf(pMapConfig, "\"angles\" \"%g %g %g\"\n", pev->angles.x, pev->angles.y, pev->angles.z);

		if (numkeys == 0)
		{
			if (!FStringNull(pev->targetname))
				fprintf(pMapConfig, "\"targetname\" \"%s\"\n", STRING(pev->targetname));
			if (!FStringNull(pev->target))
				fprintf(pMapConfig, "\"target\" \"%s\"\n", STRING(pev->target));
		}
		// Bad for items because they spawn with "in-game" scale according to game settings
		//if (pev->scale > 0.0f && pev->scale != 1.0f)
		//	fprintf(pMapConfig, "\"scale\" \"%g\"\n", pev->scale);

		// UNDONE: we don't have any GetKeyValue mechanism, so make the user do the stuff
		if (writekeynames && writekeyvalues && numkeys > 0)
		{
			for (size_t i=0; i<numkeys; ++i)
				fprintf(pMapConfig, "\"%s\" \"%s\"\n", writekeynames[i], writekeyvalues[i]);
		}

		// UNDONE: all entvars
		fprintf(pMapConfig, "}\n");
		fclose(pMapConfig);
	}
	else
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: enable if-sections in map config files. "gamerules_only == 1"
// Input  : &left - left operand
//			*cmp_operator - "=="
//			&right - right operand
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ENTCONFIG_Evaluate(const int &left, const char *cmp_operator, const int &right)
{
	if (cmp_operator == NULL)
		return false;

	if (strcmp(cmp_operator, "==") == 0)
		return (left == right);
	else if (strcmp(cmp_operator, "!=") == 0)
		return (left != right);
	else if (strcmp(cmp_operator, ">=") == 0)
		return (left >= right);
	else if (strcmp(cmp_operator, ">") == 0)
		return (left > right);
	else if (strcmp(cmp_operator, "<") == 0)
		return (left < right);
	else if (strcmp(cmp_operator, "<=") == 0)
		return (left <= right);
	else if (strcmp(cmp_operator, "&") == 0)
		return (left & right) != 0;
	else if (strcmp(cmp_operator, "^") == 0)
		return (left ^ right) != 0;
	else if (strcmp(cmp_operator, "|") == 0)
		return (left | right) !=0;
	else
		conprintf(2, "ENTCONFIG_Evaluate: unknown operator: '%s'\n", cmp_operator);

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check global conditions
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ENTCONFIG_ValidateCommand(void)
{
	if (g_MapConfigCommands)
	{
		if (g_MultiplayerOnlyCommands && gpGlobals->deathmatch == 0)
			return false;
		if (g_iGamerulesSpecificCommands != -1 && g_pGameRules && ENTCONFIG_Evaluate(g_pGameRules->GetGameType(), g_iGamerulesSpecificCommandsOp, g_iGamerulesSpecificCommands) == false)
			return false;
	}
	return true;
}
