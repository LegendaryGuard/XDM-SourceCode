#include "extdll.h"
#include "util.h"
#include "mapcycle.h"

//-----------------------------------------------------------------------------
// MAP CYCLE FUNCTIONS
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: check mapcycle.bad for this name
// Input  : *szMap - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool MapIsBanned(const char *szMap)
{
	return StringInList(szMap, "mapcycle.bad");
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server level transition
==============
*/
void ExtractCommandString(char *s, char *szCommand)
{
	// Now make rules happen
	char	pkey[512];
	char	value[512];	// use two buffers so compares work without stomping on each other
	char	*o;
	if (*s == '\\')
		++s;

	while (true)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		++s;

		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat(szCommand, pkey);
		if (strlen(value) > 0)
		{
			strcat(szCommand, " ");
			strcat(szCommand, value);
		}
		strcat(szCommand, "\n");

		if (!*s)
			return;
		++s;
	}
}


/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle(mapcycle_t *cycle)
{
	DBG_PRINTF("DestroyMapCycle()\n");
	if (cycle == NULL)
		return;

	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if (p)
	{
		start = p;
		p = p->next;
		while (p != start)
		{
			n = p->next;
			delete p;
			p = n;
		}
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}


/*
==============
ReloadMapCycleFile

Parses mapcycle.txt file into mapcycle_t structure
==============
*/
uint32 ReloadMapCycleFile(const char *filename, mapcycle_t *cycle)
{
	DBG_PRINTF("ReloadMapCycleFile(%s)\n", filename);
	if (filename == NULL || cycle == NULL)
		return 0;

	char szBuffer[MAX_RULE_BUFFER];
	char szMap[MAX_MAPNAME];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char *)LOAD_FILE_FOR_ME((char *)filename, &length);
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;
	cycle->next_item = NULL;// XDM
	uint32 count = 0;

	if (pFileList && length)
	{
		// the first map name in the file becomes the default
		while (true)
		{
			hasbuffer = 0;
			memset(szBuffer, 0, MAX_RULE_BUFFER);

			pFileList = COM_Parse(pFileList);
			if (strlen(COM_Token()) <= 0)
				break;

			strncpy(szMap, COM_Token(), MAX_MAPNAME-1);

			// Any more tokens on this line?
			if (COM_TokenWaiting(pFileList))
			{
				pFileList = COM_Parse(pFileList);
				if (strlen(COM_Token()) > 0)
				{
					hasbuffer = 1;
					strncpy(szBuffer, COM_Token(), MAX_RULE_BUFFER);// XDM: buffer overrun protection
				}
			}

			// Check map
			if (IS_MAP_VALID(szMap))
			{
				// Create entry
				char *s = NULL;
				item = new mapcycle_item_s;
				++count;
				strncpy(item->mapname, szMap, MAX_MAPNAME-1);

				// XDM: a good place to find current map
				if (cycle->next_item == NULL)
				{
					if (_stricmp(szMap, STRING(gpGlobals->mapname)) == 0)
					{
						conprintf(0, "Map cycle (%s): current map %s found in '%s' (%d)\n", filename, STRING(gpGlobals->mapname), filename, count);
						cycle->next_item = item;// just set this to this->next at the end
					}
				}

				item->minplayers = 0;
				item->maxplayers = 0;
				memset(item->rulebuffer, 0, MAX_RULE_BUFFER);
				if (hasbuffer)
				{
					s = GET_INFO_KEY_VALUE(szBuffer, "minplayers");
					if (s && s[0])
					{
						item->minplayers = clamp(atoi(s), 0, gpGlobals->maxClients);// XDM3038a
						INFO_REMOVE_KEY(szBuffer, "minplayers");
					}
					s = GET_INFO_KEY_VALUE(szBuffer, "maxplayers");
					if (s && s[0])
					{
						item->maxplayers = clamp(atoi(s), 0, gpGlobals->maxClients);// XDM3038a
						INFO_REMOVE_KEY(szBuffer, "maxplayers");
					}
					strcpy(item->rulebuffer, szBuffer);
				}
				item->next = cycle->items;
				cycle->items = item;
			}
			else
				SERVER_PRINTF("ReloadMapCycleFile(): Skipping \"%s\" from mapcycle, not a valid map\n", szMap);
		}
		FREE_FILE(aFileList);
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while (item)
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if (!item)
	{
		SERVER_PRINTF("ReloadMapCycleFile(): did not parse anything\n");
		return 0;
	}
	while (item->next)
		item = item->next;

	item->next = cycle->items;

	if (cycle->next_item)// XDM: current map found, use it's next
		cycle->next_item = cycle->next_item->next;

	if (cycle->next_item == NULL)// supersafety
		cycle->next_item = item->next;

	SERVER_PRINTF("ReloadMapCycleFile(): %u items loaded\n", count);
	return count;
}

// XDM3034
mapcycle_item_t *Mapcycle_Find(mapcycle_t *cycle, const char *map)
{
	mapcycle_item_t *start = cycle->items;
	if (start)
	{
		mapcycle_item_t *current = start;
		do
		{
			if (_stricmp(map, current->mapname) == 0)
				return current;

			current = current->next;
		}
		while (current != start);
	}
	return NULL;
}
