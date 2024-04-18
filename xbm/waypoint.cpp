#ifndef __linux__
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"
#include "bot.h"
#include "pm_shared.h"// XDM3037: VEC_HULL_*


// waypoints with information bits (flags)
WAYPOINT waypoints[MAX_WAYPOINTS];

// number of waypoints currently in use
uint32 num_waypoints = 0;
uint32 route_num_waypoints = 0;

// declare the array of head pointers to the path structures...
PATH *paths[MAX_WAYPOINTS];

// time that this waypoint was displayed (while editing)
float wp_display_time[MAX_WAYPOINTS];
float f_path_time = 0.0;

// non-team waypoints and paths are in team 1 path. kind of not good.
uint16 *shortest_path[MAX_TEAMS] = {NULL, NULL, NULL, NULL};// XDM3038c: The concept of different waypoints of different teams is wrong! All bots can use all waypoints! It is their target what differs.
uint16 *from_to[MAX_TEAMS] = {NULL, NULL, NULL, NULL};

Vector last_waypoint;
bool g_waypoint_paths = false;  // have any paths been allocated?
bool g_waypoint_on = false;
bool g_auto_waypoint = false;
bool g_path_waypoint = false;
bool g_path_waypoint_enable = true;

static FILE *fp;


void WaypointDebug(void)
{
	DBG_PRINTF("WaypointDebug: LINKED LIST ERROR!!!\n");
	DBG_FORCEBREAK
}

// free the linked list of waypoint path nodes...
void WaypointFree(void)
{
	for (int i=0; i < MAX_WAYPOINTS; ++i)
	{
#if defined (_DEBUG)
		int count = 0;
#endif
		if (paths[i])
		{
			PATH *p = paths[i];
			PATH *p_next;
			while (p)  // free the linked list
			{
				p_next = p->next;  // save the link to next
				free(p);
				p = p_next;
#if defined (_DEBUG)
				count++;
				if (count > 1000) WaypointDebug();
#endif
			}
			paths[i] = NULL;
		}
	}
}

// initialize the waypoint structures...
void WaypointInit(void)
{
	int i;
	// have any waypoint path nodes been allocated yet?
	if (g_waypoint_paths)
		WaypointFree();  // must free previously allocated path memory

	for (i=0; i < MAX_TEAMS; i++)
	{
		if (shortest_path[i] != NULL)
			free(shortest_path[i]);
		
		if (from_to[i] != NULL)
			free(from_to[i]);
	}

	for (i=0; i < MAX_WAYPOINTS; ++i)
	{
		waypoints[i].flags = 0;
		waypoints[i].origin.Clear();
		wp_display_time[i] = 0.0;
		paths[i] = NULL;  // no paths allocated yet
	}

	f_path_time = 0.0;  // reset waypoint path display time
	num_waypoints = 0;
	last_waypoint.Clear();

	for (i=0; i < MAX_TEAMS; i++)
	{
		shortest_path[i] = NULL;
		from_to[i] = NULL;
	}
}

// team_id = 1...4
bool WaypointIsForTeam(WAYPOINT *pWP, TEAM_ID team_id)
{
	if (pWP->flags & W_FL_TEAM_SPECIFIC)
		return (pWP->flags & W_FL_TEAM_BITMASK) == ((uint32)team_id-1);

	//if (team_id == TEAM_NONE)// ??
	return true;
}


void WaypointAddPath(waypointindex_t add_index, waypointindex_t path_index)
{
	PATH *p, *prev;
	waypointindex_t i;
#if defined (_DEBUG)
	waypointindex_t count = 0;
#endif
	p = paths[add_index];
	prev = NULL;

	// find an empty slot for new path_index...
	while (p != NULL)
	{
		i = 0;
		while (i < MAX_PATH_INDEX)
		{
			if (p->index[i] == WAYPOINT_NONE)
			{
				p->index[i] = path_index;
				return;
			}
			i++;
		}

		prev = p;		// save the previous node in linked list
		p = p->next;	// go to next node in linked list

#if defined (_DEBUG)
		count++;
		if (count > 100) WaypointDebug();
#endif
	}

	p = (PATH *)malloc(sizeof(PATH));

	if (p == NULL)
	{
		ALERT(at_error, "XBM: Error allocating memory for path!\n");
		return;
	}

	p->index[0] = path_index;
	p->index[1] = WAYPOINT_NONE;
	p->index[2] = WAYPOINT_NONE;
	p->index[3] = WAYPOINT_NONE;
	p->next = NULL;

	if (prev != NULL)
		prev->next = p;  // link new node into existing list

	if (paths[add_index] == NULL)
		paths[add_index] = p;  // save head point if necessary
}


void WaypointDeletePath(waypointindex_t del_index)
{
	PATH *p;
	waypointindex_t index, i;
	// search all paths for this index...
	for (index=0; index < num_waypoints; index++)
	{
		p = paths[index];
#if defined (_DEBUG)
		int count = 0;
#endif
		// search linked list for del_index...
		while (p != NULL)
		{
			i = 0;
			while (i < MAX_PATH_INDEX)
			{
				if (p->index[i] == del_index)
					p->index[i] = WAYPOINT_NONE;  // unassign this path

				i++;
			}
			p = p->next;  // go to next node in linked list

#if defined (_DEBUG)
			count++;
			if (count > 100) WaypointDebug();
#endif
		}
	}
}

void WaypointDeletePath(waypointindex_t path_index, waypointindex_t del_index)
{
	PATH *p;
	uint32 i;
#if defined (_DEBUG)
	waypointindex_t count = 0;
#endif
	p = paths[path_index];
	// search linked list for del_index...
	while (p != NULL)
	{
		i = 0;
		while (i < MAX_PATH_INDEX)
		{
			if (p->index[i] == del_index)
				p->index[i] = WAYPOINT_NONE;  // unassign this path

			++i;
		}

		p = p->next;  // go to next node in linked list

#if defined (_DEBUG)
		count++;
		if (count > 100) WaypointDebug();
#endif
	}
}


// find a path from the current waypoint. (pPath MUST be NULL on the
// initial call. subsequent calls will return other paths if they exist.)
waypointindex_t WaypointFindPath(PATH **pPath, int *path_index, waypointindex_t waypoint_index, TEAM_ID team_id)
{
	waypointindex_t index;
#if defined (_DEBUG)
	waypointindex_t count = 0;
#endif
	if (*pPath == NULL)
	{
		*pPath = paths[waypoint_index];
		*path_index = 0;
	}

	if (*path_index == MAX_PATH_INDEX)
	{
		*path_index = 0;
		*pPath = (*pPath)->next;  // go to next node in linked list
	}

	while (*pPath != NULL)
	{
		while (*path_index < MAX_PATH_INDEX)
		{
			if ((*pPath)->index[*path_index] != WAYPOINT_NONE)  // found a path?
			{
				// save the return value
				index = (*pPath)->index[*path_index];
				// skip this path if next waypoint is team specific and NOT this team
				if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
				{
					(*path_index)++;
					continue;
				}
				// set up stuff for subsequent calls...
				(*path_index)++;
				return index;
			}
			(*path_index)++;
		}
		*path_index = 0;
		*pPath = (*pPath)->next;  // go to next node in linked list

#if defined (_DEBUG)
		count++;
		if (count > 100) WaypointDebug();
#endif
	}
	return WAYPOINT_NONE;
}

// find the nearest waypoint to the player and return the index (WAYPOINT_NONE if not found)
waypointindex_t WaypointFindNearest(edict_t *pEntity, float range, TEAM_ID team_id)
{
	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	DBG_BOT_PRINT("WaypointFindNearest(%s, range %g, team %hd)\n", STRING(pEntity->v.netname), range, team_id);
	uint32 index;
	uint32 min_index;
	float distance;
	float min_distance;
	TraceResult tr;
	// find the nearest waypoint...
	min_index = WAYPOINT_NONE;
	min_distance = 9999.0;
	for (index=0; index < num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		distance = (waypoints[index].origin - pEntity->v.origin).Length();
		if ((distance < min_distance) && (distance < range))
		{
			// if waypoint is visible from current position (even behind head)...
			UTIL_TraceLine(pEntity->v.origin + pEntity->v.view_ofs, waypoints[index].origin, ignore_monsters, pEntity->v.pContainingEntity, &tr);
			if (tr.flFraction >= 1.0)
			{
				min_index = index;
				min_distance = distance;
			}
		}
	}
	return min_index;
}


// find the nearest waypoint to the source postition and return the index
waypointindex_t WaypointFindNearest(const Vector &v_src, edict_t *pEntity, float range, TEAM_ID team_id)
{
	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	DBG_BOT_PRINT("WaypointFindNearest2(%s, range %g, team %hd)\n", STRING(pEntity->v.netname), range, team_id);
	waypointindex_t index;
	waypointindex_t min_index;
	float distance;
	float min_distance;
	TraceResult tr;
	// find the nearest waypoint...
	min_index = WAYPOINT_NONE;
	min_distance = 9999.0;
	for (index=0; index < num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		distance = (waypoints[index].origin - v_src).Length();
		if ((distance < min_distance) && (distance < range))
		{
			// if waypoint is visible from source position...
			UTIL_TraceLine(v_src, waypoints[index].origin, ignore_monsters, pEntity->v.pContainingEntity, &tr);
			if (tr.flFraction >= 1.0)
			{
				min_index = index;
				min_distance = distance;
			}
		}
	}
	return min_index;
}


waypointindex_t WaypointFindNearestGoal(edict_t *pEntity, waypointindex_t src, TEAM_ID team_id, uint32 flags)
{
	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	DBG_BOT_PRINT("WaypointFindNearestGoal(%s, team %hd)\n", STRING(pEntity->v.netname), team_id);
	waypointindex_t index;
	waypointindex_t min_index;
	int distance, min_distance;
	// find the nearest waypoint with the matching flags...
	min_index = WAYPOINT_NONE;
	min_distance = 99999;
	for (index=0; index < num_waypoints; index++)
	{
		if (index == src)
			continue;  // skip the source waypoint

		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		if ((waypoints[index].flags & flags) != flags)
			continue;  // skip this waypoint if the flags don't match

		distance = WaypointRouteFromTo(src, index, team_id, true);
		if (distance == USHRT_MAX)
			continue;// XDM3038a: ?!?!?!?!

		if (distance < min_distance)
		{
			min_index = index;
			min_distance = distance;
		}
	}
	return min_index;
}

waypointindex_t WaypointFindNearestGoal(edict_t *pEntity, uint32 src, TEAM_ID team_id, uint32 flags, uint32 exclude[])
{
	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	DBG_BOT_PRINT("WaypointFindNearestGoal2(%s, team %hd)\n", STRING(pEntity->v.netname), team_id);
	waypointindex_t index;
	waypointindex_t min_index;
	int distance, min_distance;
	int exclude_index;
	// find the nearest waypoint with the matching flags...
	min_index = WAYPOINT_NONE;
	min_distance = 99999;
	for (index=0; index < num_waypoints; index++)
	{
		if (index == src)
			continue;  // skip the source waypoint

		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		if ((waypoints[index].flags & flags) != flags)
			continue;  // skip this waypoint if the flags don't match

		exclude_index = 0;
		while (exclude[exclude_index])
		{
			if (index == exclude[exclude_index])
				break;  // found a match, break out of while loop
			
			exclude_index++;
		}

		if (index == exclude[exclude_index])
			continue;  // skip any index that matches exclude list

		distance = WaypointRouteFromTo(src, index, team_id, true);
		if (distance == USHRT_MAX)
			continue;// XDM3038a: ?!?!?!?!

		if (distance < min_distance)
		{
			min_index = index;
			min_distance = distance;
		}
	}
	return min_index;
}


/*waypointindex_t WaypointFindNearestGoal(Vector v_src, edict_t *pEntity, float range, TEAM_ID team_id, int flags)
{
	waypointindex_t index, min_index;
	float distance, min_distance;

	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	// find the nearest waypoint with the matching flags...
	min_index = WAYPOINT_NONE;
	min_distance = 99999;
	for (index=0; index < num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		if ((waypoints[index].flags & flags) != flags)
			continue;  // skip this waypoint if the flags don't match

		distance = (waypoints[index].origin - v_src).Length();
		if ((distance < range) && (distance < min_distance))
		{
			min_index = index;
			min_distance = distance;
		}
	}
	return min_index;
}*/


/*waypointindex_t WaypointFindRandomGoal(edict_t *pEntity, TEAM_ID team_id, int flags)
{
	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	waypointindex_t index;
	waypointindex_t indexes[200];
	waypointindex_t count = 0;
	// find all the waypoints with the matching flags...
	for (index=0; index < num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		if ((waypoints[index].flags & flags) != flags)
			continue;  // skip this waypoint if the flags don't match

		if (count < 200)
		{
			indexes[count] = index;
			count++;
		}
	}

	if (count == 0)  // no matching waypoints found
		return WAYPOINT_NONE;

	index = RANDOM_LONG(1, count)-1;
	return indexes[index];
}*/


waypointindex_t WaypointFindRandomGoal(edict_t *pEntity, TEAM_ID team_id, uint32 flags, uint32 exclude[])
{
	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	waypointindex_t index;
	waypointindex_t indexes[200];
	waypointindex_t count = 0;
	waypointindex_t exclude_index;
	// find all the waypoints with the matching flags...
	for (index=0; index < num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		if ((waypoints[index].flags & flags) != flags)
			continue;  // skip this waypoint if the flags don't match

		exclude_index = 0;
		while (exclude[exclude_index])
		{
			if (index == exclude[exclude_index])
				break;  // found a match, break out of while loop
			
			exclude_index++;
		}

		if (index == exclude[exclude_index])
			continue;  // skip any index that matches exclude list

		if (count < 200)
		{
			indexes[count] = index;
			count++;
		}
	}

	if (count == 0)  // no matching waypoints found
		return WAYPOINT_NONE;

	index = RANDOM_LONG(1, count)-1;
	return indexes[index];
}


/*waypointindex_t WaypointFindRandomGoal(Vector v_src, edict_t *pEntity, float range, TEAM_ID team_id, int flags)
{
	if (num_waypoints < 1)
		return WAYPOINT_NONE;

	waypointindex_t index;
	waypointindex_t indexes[200];
	waypointindex_t count = 0;
	float distance;
	// find all the waypoints with the matching flags...
	for (index=0; index < num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		if ((waypoints[index].flags & flags) != flags)
			continue;  // skip this waypoint if the flags don't match

		distance = (waypoints[index].origin - v_src).Length();
		if ((distance < range) && (count < 200))
		{
			indexes[count] = index;
			count++;
		}
	}

	if (count == 0)  // no matching waypoints found
		return WAYPOINT_NONE;

	index = RANDOM_LONG(1, count)-1;
	return indexes[index];
}*/


waypointindex_t WaypointFindNearestAiming(const Vector &v_origin)
{
	waypointindex_t min_index = WAYPOINT_NONE;
	if (num_waypoints >= 1)
	{
		waypointindex_t index;
		float min_distance = 9999.0f;
		float distance;
		// search for nearby aiming waypoint...
		for (index=0; index < num_waypoints; index++)
		{
			if (waypoints[index].flags & W_FL_DELETED)
				continue;  // skip any deleted waypoints

			if ((waypoints[index].flags & W_FL_AIMING) == 0)
				continue;  // skip any NON aiming waypoints

			distance = (v_origin - waypoints[index].origin).Length();
			if ((distance < min_distance) && (distance < 40))
			{
				min_index = index;
				min_distance = distance;
			}
		}
	}
	return min_index;
}


void WaypointDrawBeam(edict_t *pEntity, Vector start, Vector end, int width, int noise, int red, int green, int blue, int brightness, int speed)
{
	MESSAGE_BEGIN(MSG_ONE, svc_temp_entity, NULL, pEntity);
		WRITE_BYTE(TE_BEAMPOINTS);
		WRITE_COORD(start.x);
		WRITE_COORD(start.y);
		WRITE_COORD(start.z);
		WRITE_COORD(end.x);
		WRITE_COORD(end.y);
		WRITE_COORD(end.z);
		WRITE_SHORT(g_iModelIndexLaser);
		WRITE_BYTE(1); // framestart
		WRITE_BYTE(10); // framerate
		WRITE_BYTE(10); // life in 0.1's
		WRITE_BYTE(width);// width
		WRITE_BYTE(noise);// noise
		WRITE_BYTE(red);// r, g, b
		WRITE_BYTE(green);// r, g, b
		WRITE_BYTE(blue);// r, g, b
		WRITE_BYTE(brightness);// brightness
		WRITE_BYTE(speed);// speed
	MESSAGE_END();
}

// OBSOLETE!!!!!!!!! Rewrite!
void WaypointSearchItems(edict_t *pEntity, const Vector &origin, waypointindex_t wpt_index)
{
	edict_t *pent = NULL;
	float radius = 40;
	TraceResult tr;
	float distance;
	float min_distance;
	char item_name[64];
	char nearest_name[64];
	edict_t *nearest_pent;
//	int tfc_backpack_index;

	nearest_name[0] = 0;		// null out nearest_name string
//	tfc_backpack_index = WAYPOINT_NONE;	// "null" out backpack index
	nearest_pent = NULL;
	min_distance = 9999.0;
	
	//********************************************************
	// look for the nearest health, armor, ammo, weapon, etc.
	//********************************************************
	while ((pent = UTIL_FindEntityInSphere( pent, origin, radius )) != NULL)
	{
		if (pEntity)
			UTIL_TraceLine( origin, pent->v.origin, ignore_monsters, pEntity->v.pContainingEntity, &tr);
		else
			UTIL_TraceLine( origin, pent->v.origin, ignore_monsters, NULL, &tr);

		// make sure entity is visible...
		if (tr.flFraction >= 1.0)
		{
			
			if (mod_id == GAME_XDM_DLL && XDM_EntityIs)
			{
				int entitytype = XDM_EntityIs(pent);
				if ((entitytype == ENTIS_PLAYERWEAPON) && (pent->v.owner == NULL))
				{
					waypoints[wpt_index].flags |= W_FL_WEAPON;
				}
				else if ((entitytype == ENTIS_PLAYERITEM) && (pent->v.owner == NULL))
				{
					if (strcmp("item_health", STRING(pent->v.classname)) == 0)// WRONG! it's not a player item, btw.
					{
						waypoints[wpt_index].flags |= W_FL_HEALTH;
					}
					else if (strcmp("item_battery", STRING(pent->v.classname)) == 0)
					{
						waypoints[wpt_index].flags |= W_FL_ARMOR;
					}
				}
				else if ((entitytype == ENTIS_GAMEGOAL) && (pent->v.aiment == NULL))
				{
					waypoints[wpt_index].flags |= W_FL_FLAG_GOAL;
				}

				if ((entitytype == ENTIS_PLAYERWEAPON || entitytype == ENTIS_PLAYERITEM) && (pent->v.owner == NULL))
				{
					distance = (pent->v.origin - origin).Length();
					if (distance < min_distance)
					{
						strcpy(nearest_name, item_name);
//						tfc_backpack_index = WAYPOINT_NONE;  // "null" out backpack index
						nearest_pent = pent;
						min_distance = distance;
					}
				}
			}
			else
			{
				strcpy(item_name, STRING(pent->v.classname));
				if ((strncmp("item_health", item_name, 11) == 0) ||
					(strncmp("item_armor", item_name, 10) == 0) ||
					(strncmp("ammo_", item_name, 5) == 0) ||
					(strcmp("item_cells", item_name) == 0) ||
					(strcmp("item_shells", item_name) == 0) ||
					(strcmp("item_spikes", item_name) == 0) ||
					(strcmp("item_rockets", item_name) == 0) ||
					((strncmp("weapon_", item_name, 7) == 0) &&
					(pent->v.owner == NULL)))
				{
					distance = (pent->v.origin - origin).Length();
					if (distance < min_distance)
					{
						strcpy(nearest_name, item_name);
//						tfc_backpack_index = WAYPOINT_NONE;  // "null" out backpack index
						nearest_pent = pent;
						min_distance = distance;
					}
				}
			}
		}
	}

	if (nearest_name[0])  // found an entity name
	{
		if (strncmp("item_health", nearest_name, 11) == 0)
		{
			if (pEntity)
				ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "found a healthkit!\n");
			waypoints[wpt_index].flags |= W_FL_HEALTH;
		}

		if (strncmp("item_battery", nearest_name, 10) == 0)
		{
			if (pEntity)
				ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "found some armor!\n");
			waypoints[wpt_index].flags |= W_FL_ARMOR;
		}

		if ((strncmp("ammo_", nearest_name, 5) == 0) ||
			(strcmp("item_cells", nearest_name) == 0) ||
			(strcmp("item_shells", nearest_name) == 0) ||
			(strcmp("item_spikes", nearest_name) == 0) ||
			(strcmp("item_rockets", nearest_name) == 0))
		{
			if (pEntity)
				ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "found some ammo!\n");
			waypoints[wpt_index].flags |= W_FL_AMMO;
		}

		if ((strncmp("weapon_", nearest_name, 7) == 0) &&
			(nearest_pent->v.owner == NULL))
		{
			if (pEntity)
				ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "found a weapon!\n");
			waypoints[wpt_index].flags |= W_FL_WEAPON;
		}
	}
}


void WaypointAdd(edict_t *pEntity)
{
	if (num_waypoints >= MAX_WAYPOINTS)
		return;

	waypointindex_t index = 0;
	// find the next available slot for the new waypoint...
	while (index < num_waypoints)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			break;
		
		index++;
	}

	waypoints[index].flags = 0;
	// store the origin (location) of this waypoint (use entity origin)
	waypoints[index].origin = pEntity->v.origin;
	// store the last used waypoint for the auto waypoint code...
	last_waypoint = pEntity->v.origin;
	// set the time that this waypoint was originally displayed...
	wp_display_time[index] = gpGlobals->time;

	Vector start, end;

	start = pEntity->v.origin - Vector(0, 0, 34);
	end = start + Vector(0, 0, 68);
	
	if ((pEntity->v.flags & FL_DUCKING) == FL_DUCKING)
	{
		waypoints[index].flags |= W_FL_CROUCH;  // crouching waypoint
		
		start = pEntity->v.origin - Vector(0, 0, 17);
		end = start + Vector(0, 0, 34);
	}
	
	if (IsOnLadder(pEntity))
		waypoints[index].flags |= W_FL_LADDER;  // waypoint on a ladder

	// search the area near the waypoint for items (HEALTH, AMMO, WEAPON, etc.)
	WaypointSearchItems(pEntity, waypoints[index].origin, index);
	
	// draw a blue waypoint
	WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	// increment total number of waypoints if adding at end of array...
	if (index == num_waypoints)
		num_waypoints++;
	
	// calculate all the paths to this new waypoint
	for (waypointindex_t i=0; i < num_waypoints; i++)
	{
		if (i == index)
			continue;  // skip the waypoint that was just added
		
		if (waypoints[i].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints
		
		// check if the waypoint is reachable from the new one (one-way)
		if ( WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity) &&
			g_path_waypoint_enable)
		{
			WaypointAddPath(index, i);
		}
		
		// check if the new one is reachable from the waypoint (other way)
		if ( WaypointReachable(waypoints[i].origin, pEntity->v.origin, pEntity) &&
			g_path_waypoint_enable)
		{
			WaypointAddPath(i, index);
		}
	}
}


void WaypointAddAiming(edict_t *pEntity)
{
	if (num_waypoints >= MAX_WAYPOINTS)
		return;

	waypointindex_t index = 0;

	// find the next available slot for the new waypoint...
	while (index < num_waypoints)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			break;

		index++;
	}

	waypoints[index].flags = W_FL_AIMING;  // aiming waypoint

	Vector v_angle(pEntity->v.v_angle);
	v_angle.x = 0;  // reset pitch to horizontal
	v_angle.z = 0;  // reset roll to level

	UTIL_MakeVectors(v_angle);

	// store the origin (location) of this waypoint (use entity origin)
	waypoints[index].origin = pEntity->v.origin + gpGlobals->v_forward * 25.0f;

	// set the time that this waypoint was originally displayed...
	wp_display_time[index] = gpGlobals->time;

	Vector start, end;
	start = pEntity->v.origin - Vector(0, 0, 10);
	end = start + Vector(0, 0, 14);

	// draw a blue waypoint
	WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);

	// increment total number of waypoints if adding at end of array...
	if (index == num_waypoints)
		num_waypoints++;
}


void WaypointDelete(edict_t *pEntity)
{
	if (num_waypoints < 1)
		return;

	waypointindex_t index = WaypointFindNearest(pEntity, 50.0, TEAM_NONE);
	if (index == WAYPOINT_NONE)
		return;

	if ((waypoints[index].flags & W_FL_SNIPER) ||
		(waypoints[index].flags & W_FL_SENTRYGUN) ||
		(waypoints[index].flags & W_FL_DISPENSER) ||
		(waypoints[index].flags & W_FL_JUMP))
	{
		waypointindex_t i;
		waypointindex_t min_index = WAYPOINT_NONE;
		float min_distance = 9999.0f;
		float distance;

		// search for nearby aiming waypoint and delete it also...
		for (i=0; i < num_waypoints; i++)
		{
			if (waypoints[i].flags & W_FL_DELETED)
				continue;  // skip any deleted waypoints

			if ((waypoints[i].flags & W_FL_AIMING) == 0)
				continue;  // skip any NON aiming waypoints

			distance = (waypoints[i].origin - waypoints[index].origin).Length();

			if ((distance < min_distance) && (distance < 40))
			{
				min_index = i;
				min_distance = distance;
			}
		}

		if (min_index != WAYPOINT_NONE)
		{
			waypoints[min_index].flags = W_FL_DELETED;  // not being used
			waypoints[min_index].origin.Clear();
			wp_display_time[min_index] = 0.0;
		}
	}

	// delete any paths that lead to this index...
	WaypointDeletePath(index);

#if defined (_DEBUG)
	int count = 0;
#endif

	// free the path for this index...
	if (paths[index] != NULL)
	{
		PATH *p = paths[index];
		PATH *p_next;
		while (p)  // free the linked list
		{
			p_next = p->next;  // save the link to next
			free(p);
			p = p_next;

#if defined (_DEBUG)
			count++;
			if (count > 100) WaypointDebug();
#endif
		}
		paths[index] = NULL;
	}
	waypoints[index].flags = W_FL_DELETED;  // not being used
	waypoints[index].origin.Clear();
	wp_display_time[index] = 0.0;
	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/mine_activate.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
}

void WaypointUpdate(void)
{
	uint32 mask = W_FL_HEALTH | W_FL_ARMOR | W_FL_AMMO | W_FL_WEAPON;
	for (size_t index=0; index < num_waypoints; ++index)
	{
		waypoints[index].flags &= ~mask;  // clear the mask bits
		WaypointSearchItems(NULL, waypoints[index].origin, index);
	}
}

// allow player to manually create a path from one waypoint to another
void WaypointCreatePath(edict_t *pEntity, int cmd)
{
	static waypointindex_t waypoint1 = WAYPOINT_NONE;  // initialized to unassigned
	static waypointindex_t waypoint2 = WAYPOINT_NONE;  // initialized to unassigned

	if (cmd == 1)  // assign source of path
	{
		waypoint1 = WaypointFindNearest(pEntity, 50.0, TEAM_NONE);
		if (waypoint1 == WAYPOINT_NONE)
		{
			// play "cancelled" sound...
			EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			return;
		}
		// play "start" sound...
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		return;
	}

	if (cmd == 2)  // assign dest of path and make path
	{
		waypoint2 = WaypointFindNearest(pEntity, 50.0, TEAM_NONE);
		if ((waypoint1 == WAYPOINT_NONE) || (waypoint2 == WAYPOINT_NONE))
		{
			// play "error" sound...
			EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			return;
		}
		WaypointAddPath(waypoint1, waypoint2);
		// play "done" sound...
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	}
}

// allow player to manually remove a path from one waypoint to another
void WaypointRemovePath(edict_t *pEntity, int cmd)
{
	static waypointindex_t waypoint1 = WAYPOINT_NONE;  // initialized to unassigned
	static waypointindex_t waypoint2 = WAYPOINT_NONE;  // initialized to unassigned

	if (cmd == 1)  // assign source of path
	{
		waypoint1 = WaypointFindNearest(pEntity, 50.0, TEAM_NONE);
		if (waypoint1 == WAYPOINT_NONE)
		{
			// play "cancelled" sound...
			EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			return;
		}
		// play "start" sound...
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		return;
	}

	if (cmd == 2)  // assign dest of path and make path
	{
		waypoint2 = WaypointFindNearest(pEntity, 50.0, TEAM_NONE);
		if ((waypoint1 == WAYPOINT_NONE) || (waypoint2 == WAYPOINT_NONE))
		{
			// play "error" sound...
			EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
			return;
		}
		WaypointDeletePath(waypoint1, waypoint2);
		// play "done" sound...
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	}
}

bool WaypointReachable(Vector v_src, Vector v_dest, edict_t *pEntity)
{
	TraceResult tr;
	float curr_height, last_height;
	float distance = (v_dest - v_src).Length();
	// is the destination close enough?
	if (distance < REACHABLE_RANGE)
	{
		// check if this waypoint is "visible"...
		UTIL_TraceLine(v_src, v_dest, ignore_monsters, pEntity->v.pContainingEntity, &tr);
		// if waypoint is visible from current position (even behind head)...
		if (tr.flFraction >= 1.0)
		{
			// check for special case of both waypoints being underwater...
			if ((POINT_CONTENTS(v_src) == CONTENTS_WATER) && (POINT_CONTENTS(v_dest) == CONTENTS_WATER))
				return true;

			// check for special case of waypoint being suspended in mid-air...
			// is dest waypoint higher than src? (45 is max jump height)
			if (v_dest.z > (v_src.z + 45.0))
			{
				Vector v_new_src = v_dest;
				Vector v_new_dest = v_dest;
				v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units
				UTIL_TraceLine(v_new_src, v_new_dest, dont_ignore_monsters, pEntity->v.pContainingEntity, &tr);
				// check if we didn't hit anything, if not then it's in mid-air
				if (tr.flFraction >= 1.0)
					return false;  // can't reach this one
			}
			// check if distance to ground increases more than jump height
			// at points between source and destination...
			Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
			Vector v_check = v_src;
			Vector v_down = v_src;
			v_down.z -= 1000.0f;  // straight down 1000 units
			UTIL_TraceLine(v_check, v_down, ignore_monsters, pEntity->v.pContainingEntity, &tr);
			last_height = tr.flFraction * 1000.0f;  // height from ground
			distance = (v_dest - v_check).Length();  // distance from goal
			while (distance > 10.0)
			{
				// move 10 units closer to the goal...
				v_check = v_check + (v_direction * 10.0);
				v_down = v_check;
				v_down.z -= 1000.0f;  // straight down 1000 units
				UTIL_TraceLine(v_check, v_down, ignore_monsters, pEntity->v.pContainingEntity, &tr);
				curr_height = tr.flFraction * 1000.0f;  // height from ground
				// is the difference in the last height and the current height
				// higher that the jump height?
				if ((last_height - curr_height) > 45.0)
				{
					// can't get there from here...
					return false;
				}
				last_height = curr_height;
				distance = (v_dest - v_check).Length();  // distance from goal
			}
			return true;
		}
	}
	return false;
}

// find the nearest reachable waypoint
waypointindex_t WaypointFindReachable(edict_t *pEntity, float range, TEAM_ID team_id)
{
	waypointindex_t index, min_index = 0;
	float distance;
	float min_distance;
	TraceResult tr;
	// find the nearest waypoint...
	min_distance = 9999.0;
	for (index=0; index < num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		if (waypoints[index].flags & W_FL_AIMING)
			continue;  // skip any aiming waypoints

		// skip this waypoint if it's team specific and teams don't match...
		if (IsTeamplay() && (team_id != TEAM_NONE) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) && !WaypointIsForTeam(&waypoints[index], team_id))
			continue;

		distance = (waypoints[index].origin - pEntity->v.origin).Length();
		if (distance < min_distance)
		{
			// if waypoint is visible from current position (even behind head)...
			UTIL_TraceLine(pEntity->v.origin + pEntity->v.view_ofs, waypoints[index].origin, ignore_monsters, pEntity->v.pContainingEntity, &tr);
			if (tr.flFraction >= 1.0)
			{
				if (WaypointReachable(pEntity->v.origin, waypoints[index].origin, pEntity))
				{
					min_index = index;
					min_distance = distance;
				}
			}
		}
	}

	// if not close enough to a waypoint then just return
	if (min_distance > range)
		return WAYPOINT_NONE;

	return min_index;
}


void WaypointPrintInfo(edict_t *pEntity)
{
	// find the nearest waypoint...
	uint32 index = WaypointFindNearest(pEntity, 50.0, TEAM_NONE);
	if (/*index == WAYPOINT_NONE || */index >= MAX_WAYPOINTS)
		return;

	char msg[80];
	_snprintf(msg, 80, "Waypoint %ud of %d total\n", index, num_waypoints);
	ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, msg);

	int flags = waypoints[index].flags;

	if (flags & W_FL_TEAM_SPECIFIC)
	{
		_snprintf(msg, 80, "Waypoint is for TEAM %d\n", (flags & W_FL_TEAM_BITMASK)+1);
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, msg);
	}

	if (flags & W_FL_LIFT)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "Bot will wait for lift before approaching\n");

	if (flags & W_FL_LADDER)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "This waypoint is on a ladder\n");

	if (flags & W_FL_DOOR)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "This is a door waypoint\n");

	if (flags & W_FL_HEALTH)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "There is health near this waypoint\n");

	if (flags & W_FL_ARMOR)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "There is armor near this waypoint\n");

	if (flags & W_FL_AMMO)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "There is ammo near this waypoint\n");

	if (flags & W_FL_WEAPON)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "There is a weapon near this waypoint\n");

	if (flags & W_FL_JUMP)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "Bot will jump here\n");

	if (flags & W_FL_SNIPER)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "This is a sniper waypoint\n");

	if (flags & W_FL_FLAG)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "There is a flag near this waypoint\n");

	if (flags & W_FL_FLAG_GOAL)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "There is a flag goal near this waypoint\n");

	if (flags & W_FL_PRONE)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "Bot will go prone here\n");

	if (flags & W_FL_SENTRYGUN)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "Engineers will build a sentry gun here\n");

	if (flags & W_FL_DISPENSER)
		ClientPrint(&pEntity->v, HUD_PRINTNOTIFY, "Engineers will build a dispenser here\n");
}


//-----------------------------------------------------------------------------
// Purpose: Track movements of a REAL player and draw nearby waypoints?
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void WaypointThink(edict_t *pEntity)
{
	float distance, min_distance;
	Vector start, end;
	waypointindex_t i, index = 0;

	if (g_auto_waypoint)  // is auto waypoint on?
	{
		// find the distance from the last used waypoint
		distance = (last_waypoint - pEntity->v.origin).Length();
		if (distance > 200)
		{
			min_distance = 9999.0;
			// check that no other reachable waypoints are nearby...
			for (i=0; i < num_waypoints; i++)
			{
				if (waypoints[i].flags & W_FL_DELETED)
					continue;

				if (waypoints[i].flags & W_FL_AIMING)
					continue;

				if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
				{
					distance = (waypoints[i].origin - pEntity->v.origin).Length();
					if (distance < min_distance)
						min_distance = distance;
				}
			}
			// make sure nearest waypoint is far enough away...
			if (min_distance >= 200)
				WaypointAdd(pEntity);  // place a waypoint here
		}
	}

	min_distance = 9999.0;

	if (g_waypoint_on)  // display the waypoints if turned on...
	{
		int amp, r,g,b;
		for (i=0; i < num_waypoints; i++)
		{
			if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
				continue;

			distance = (waypoints[i].origin - pEntity->v.origin).Length();
			if (distance < 500)
			{
				if (distance < min_distance)
				{
					index = i; // store index of nearest waypoint
					min_distance = distance;
				}

				if ((wp_display_time[i] + 1.0) < gpGlobals->time)
				{
					if (waypoints[i].flags & W_FL_CROUCH)
					{
						start = waypoints[i].origin + Vector(0, 0, DUCK_HULL_MIN);
						end = waypoints[i].origin + Vector(0, 0, DUCK_HULL_MAX);
					}
					else if (waypoints[i].flags & W_FL_AIMING)
					{
						start = waypoints[i].origin + Vector(0, 0, 10);// wtf?
						end = start + Vector(0, 0, 14);
					}
					else
					{
						start = waypoints[i].origin + Vector(0, 0, HULL_MIN);
						end = waypoints[i].origin + Vector(0, 0, HULL_MAX);
					}

					amp = 0;
					r = 0;
					g = 0;
					b = 255;
					if (waypoints[i].flags & W_FL_WEAPON)
					{
						amp = 0;
						r = 255; g = 255; b = 255;
					}
					if (waypoints[i].flags & W_FL_HEALTH)
					{
						amp = 0;
						r = 255; g = 143; b = 0;
					}
					if (waypoints[i].flags & W_FL_ARMOR)
					{
						amp = 0;
						r = 255; g = 255; b = 0;
					}
					if (waypoints[i].flags & W_FL_AMMO)
					{
						amp = 0;
						r = 63; g = 255; b = 0;
					}
					if (waypoints[i].flags & W_FL_FLAG)
					{
						amp += 20;
						g += 255;
					}
					if (waypoints[i].flags & W_FL_FLAG_GOAL)
					{
						amp += 20;
						g += 127;
					}

					// draw a blue waypoint
					WaypointDrawBeam(pEntity, start, end, 30, amp, r,g,b, 250, 5);
					wp_display_time[i] = gpGlobals->time;
				}
			}
		}
		// check if path waypointing is on...
		if (g_path_waypoint)
		{
			// check if player is close enough to a waypoint and time to draw path...
			if ((min_distance <= 50) && (f_path_time <= gpGlobals->time))
			{
				PATH *p = paths[index];
				f_path_time = gpGlobals->time + 1.0f;
				while (p != NULL)
				{
					i = 0;
					while (i < MAX_PATH_INDEX)
					{
						if (p->index[i] != WAYPOINT_NONE)
						{
							Vector v_src = waypoints[index].origin;
							Vector v_dest = waypoints[p->index[i]].origin;
							
							// draw a white line to this index's waypoint
							WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 250, 250, 250, 200, 10);
						}
						i++;
					}
					p = p->next;  // go to next node in linked list
				}
			}
		}
	}
}


void WaypointFloyds(unsigned short *a_shortest_path, unsigned short *a_from_to)
{
	unsigned int x, y, z;
	int changed = 1;
	int distance;

	for (y=0; y < route_num_waypoints; y++)
	{
		for (z=0; z < route_num_waypoints; z++)
			a_from_to[y * route_num_waypoints + z] = z;
	}

	while (changed)
	{
		changed = 0;
		for (x=0; x < route_num_waypoints; x++)
		{
			for (y=0; y < route_num_waypoints; y++)
			{
				for (z=0; z < route_num_waypoints; z++)
				{
					if ((a_shortest_path[y * route_num_waypoints + x] == WAYPOINT_UNREACHABLE) ||
						(a_shortest_path[x * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
						continue;

					distance = a_shortest_path[y * route_num_waypoints + x] + a_shortest_path[x * route_num_waypoints + z];

					if (distance > WAYPOINT_MAX_DISTANCE)
						distance = WAYPOINT_MAX_DISTANCE;

					if ((distance < a_shortest_path[y * route_num_waypoints + z]) ||
						(a_shortest_path[y * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
					{
						a_shortest_path[y * route_num_waypoints + z] = distance;
						a_from_to[y * route_num_waypoints + z] = a_from_to[y * route_num_waypoints + x];
						changed = 1;
					}
				}
			}
		}
	}
}

uint16 WaypointRouteFromTo(waypointindex_t src, waypointindex_t dest, TEAM_ID team_id, bool shortest)
{
	if (!IsValidTeam(team_id))
		return USHRT_MAX;

	int teamindex = (team_id == TEAM_NONE)?0:team_id-1;

	if (teamindex > 0 && from_to[teamindex] == NULL)// if no team specific waypoints exist
		teamindex = 0;// use team 1

	if (from_to[teamindex] == NULL)// if no route information just return
		return USHRT_MAX;

	uint16 *pReturn;
	if (shortest)
		pReturn = shortest_path[teamindex];
	else
		pReturn = from_to[teamindex];

	if (src == WAYPOINT_NONE)// XDM3038a: is this right?
		src = 0;
	if (dest == WAYPOINT_NONE)
		dest = 0;

	return pReturn[src * route_num_waypoints + dest];
}

void WaypointSave(void)
{
	char filename[256];
	char mapname[64];
	WAYPOINT_HDR header;
	uint32 index, i;
	short int num;
	PATH *p;
	strcpy(header.filetype, "HPB_bot");
	header.waypoint_file_version = WAYPOINT_VERSION;
	header.waypoint_file_flags = 0;  // not currently used
	header.number_of_waypoints = num_waypoints;
	memset(header.mapname, 0, sizeof(header.mapname));
	strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
	header.mapname[31] = 0;
	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".HPB_wpt");
	UTIL_BuildFileName(filename, "maps", mapname);
	FILE *bfp = fopen(filename, "wb");
	// write the waypoint header to the file...
	fwrite(&header, sizeof(header), 1, bfp);
	// write the waypoint data to the file...
	for (index=0; index < num_waypoints; index++)
		fwrite(&waypoints[index], sizeof(waypoints[0]), 1, bfp);

	// save the waypoint paths...
	for (index=0; index < num_waypoints; index++)
	{
		// count the number of paths from this node...
		p = paths[index];
		num = 0;
		while (p != NULL)
		{
			i = 0;
			while (i < MAX_PATH_INDEX)
			{
				if (p->index[i] != WAYPOINT_NONE)
					num++;  // count path node if it's used

				i++;
			}
			p = p->next;  // go to next node in linked list
		}
		fwrite(&num, sizeof(num), 1, bfp);  // write the count
		// now write out each path index...
		p = paths[index];
		while (p != NULL)
		{
			i = 0;
			while (i < MAX_PATH_INDEX)
			{
				if (p->index[i] != WAYPOINT_NONE)  // save path node if it's used
					fwrite(&p->index[i], sizeof(p->index[0]), 1, bfp);

				i++;
			}
			p = p->next;  // go to next node in linked list
		}
	}
	fclose(bfp);
}

bool WaypointLoad(void)
{
	FILE *bfp;
	char mapname[64];
	char filename[256];
	char new_filename[256];
#ifdef __linux__
	char cmd[512];
#endif
	WAYPOINT_HDR header;
	uint32 index, i;
	short int num;
	short int path_index;
	bool need_rename;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".HPB_wpt");

	UTIL_BuildFileName(filename, "maps", mapname);

	if (IsDedicatedServer)
		printf("loading waypoint file: %s\n", filename);

	bfp = fopen(filename, "rb");

	need_rename = false;

	// if .HBP_wpt files doesn't exist, check .wpt file...
	if (bfp == NULL)
	{
		need_rename = true;

		strcpy(mapname, STRING(gpGlobals->mapname));
		strcat(mapname, ".wpt");

		UTIL_BuildFileName(filename, "maps", mapname);

		if (IsDedicatedServer)
			printf("loading waypoint file: %s\n", filename);

		bfp = fopen(filename, "rb");
	}

	// if file exists, read the waypoint structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);
		header.filetype[7] = 0;
		if (strcmp(header.filetype, "HPB_bot") != 0)
		{
			conprintf(1, "%s is not a HPB bot waypoint file!\n", filename);
			fclose(bfp);
			return false;
		}
		if (header.waypoint_file_version != WAYPOINT_VERSION)
		{
			conprintf(1, "Incompatible HPB bot waypoint file version %d!\nWaypoints not loaded!\n", header.waypoint_file_version);
			fclose(bfp);
			return false;
		}

		header.mapname[31] = 0;

		if (strcmp(header.mapname, STRING(gpGlobals->mapname)) != 0)
		{
			conprintf(1, "%s HPB bot waypoints are not for this map!\n", filename);
			fclose(bfp);
			return false;
		}

		WaypointInit();  // remove any existing waypoints

		for (i=0; i < header.number_of_waypoints; i++)
		{
			fread(&waypoints[i], sizeof(waypoints[0]), 1, bfp);
			num_waypoints++;
		}

		// read and add waypoint paths...
		for (index=0; index < num_waypoints; index++)
		{
			// read the number of paths from this node...
			fread(&num, sizeof(num), 1, bfp);

			for (short j=0; j < num; j++)
			{
				fread(&path_index, sizeof(path_index), 1, bfp);
				WaypointAddPath(index, path_index);
			}
		}
		g_waypoint_paths = true;  // keep track so path can be freed

		fclose(bfp);

		if (need_rename)
		{
			strcpy(mapname, STRING(gpGlobals->mapname));
			strcat(mapname, ".HPB_wpt");

			UTIL_BuildFileName(new_filename, "maps", mapname);

#ifndef __linux__
			rename(filename, new_filename);
#else
			_snprintf(cmd, MAX_PATH*2, "/bin/mv -f %s %s", filename, new_filename);
			system(cmd);
#endif
		}
		WaypointRouteInit();
	}
	else
	{
		if (IsDedicatedServer)
			printf("Waypoint file %s not found!\n", filename);
		else
			SERVER_PRINT("Waypoint file not found!\n");

		return false;
	}
	return true;
}

void WaypointRouteInit(void)
{
	if (num_waypoints == 0)
		return;

	waypointindex_t index;
	unsigned int array_size;
	unsigned int row;
	unsigned int a, b;
	unsigned int num_items;
	uint32 matrix;
	int i, offset;
	float distance;
	unsigned short *pShortestPath, *pFromTo;
	FILE *bfp;
	char filename[256];
	char filename2[256];
	char mapname[64];
	char msg[80];
	bool build_matrix[MAX_TEAMS];

	// save number of current waypoints in case waypoints get added later
	route_num_waypoints = num_waypoints;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".HPB_wpt");

	UTIL_BuildFileName(filename, "maps", mapname);

	build_matrix[0] = true;  // always build matrix 0 (non-team and team 1)
	build_matrix[1] = false;
	build_matrix[2] = false;
	build_matrix[3] = false;

	//uint32 wpteam;
	// find out how many route matrixes to create...
	/* XDM3038c: this concept is WRONG! for (index=0; index < route_num_waypoints; index++)
	{
		if (waypoints[index].flags & W_FL_TEAM_SPECIFIC)
		{
			//wpteam = (waypoints[index].flags & W_FL_TEAM_BITMASK);// evaluated to TEAM_ID-1
			//build_matrix[wpteam] = true;
			build_matrix[(waypoints[index].flags & W_FL_TEAM_BITMASK)] = true;
		}
	}*/

	array_size = route_num_waypoints * route_num_waypoints;

	for (matrix=0; matrix < MAX_TEAMS; ++matrix)
	{
		if (build_matrix[matrix])
		{
			char ext_str[16];  // ".HPB_wpX\0"
			int file1, file2;
			struct stat stat1, stat2;
			_snprintf(ext_str, 16, ".HPB_wp%d\0", matrix+1);
			ext_str[15] = '\0';
			strcpy(mapname, STRING(gpGlobals->mapname));
			strcat(mapname, ext_str);
			UTIL_BuildFileName(filename2, "maps", mapname);

			if (_access(filename2, 0) == 0)// does the .HPB_wpX file exist? // The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _access.
			{
				file1 = _open(filename, O_RDONLY);// The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _open. See online help for details.	1
				file2 = _open(filename2, O_RDONLY);
				fstat(file1, &stat1);
				fstat(file2, &stat2);
				_close(file1);
				_close(file2);

				if (stat1.st_mtime < stat2.st_mtime)// is .HPB_wpt older than .HPB_wpX file?
				{
					_snprintf(msg, 80, "Loading waypoint paths for team %d\n", matrix+1);
					SERVER_PRINT(msg);

					shortest_path[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);
					if (shortest_path[matrix] == NULL)
						ALERT(at_error, "Error allocating memory for shortest path!\n");

					from_to[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);
					if (from_to[matrix] == NULL)
						ALERT(at_error, "Error allocating memory for from to matrix!\n");

					bfp = fopen(filename2, "rb");
					if (bfp != NULL)
					{
						num_items = fread(shortest_path[matrix], sizeof(unsigned short), array_size, bfp);
						if (num_items != array_size)
						{
							// if couldn't read enough data, free memory to recalculate it
							conprintf(1, "error reading enough path items, recalculating...\n");
							free(shortest_path[matrix]);
							shortest_path[matrix] = NULL;
							free(from_to[matrix]);
							from_to[matrix] = NULL;
						}
						else
						{
							num_items = fread(from_to[matrix], sizeof(unsigned short), array_size, bfp);
							if (num_items != array_size)
							{
								// if couldn't read enough data, free memory to recalculate it
								conprintf(1, "error reading enough path items, recalculating...\n");
								free(shortest_path[matrix]);
								shortest_path[matrix] = NULL;
								free(from_to[matrix]);
								from_to[matrix] = NULL;
							}
						}
					}
					else
					{
						conprintf(1, "Error reading waypoint paths!\n");
						free(shortest_path[matrix]);
						shortest_path[matrix] = NULL;
						free(from_to[matrix]);
						from_to[matrix] = NULL;
					}
					fclose(bfp);
				}
			}

			if (shortest_path[matrix] == NULL)
			{
				_snprintf(msg, 80, "Calculating waypoint paths for team %d...\n", matrix+1);
				SERVER_PRINT(msg);

				shortest_path[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);
				if (shortest_path[matrix] == NULL)
					ALERT(at_error, "Error allocating memory for shortest path!");

				from_to[matrix] = (unsigned short *)malloc(sizeof(unsigned short) * array_size);
				if (from_to[matrix] == NULL)
					ALERT(at_error, "Error allocating memory for from to matrix!");

				pShortestPath = shortest_path[matrix];
				pFromTo = from_to[matrix];

				//for (index=0; index < array_size; index++)
				//	pShortestPath[index] = WAYPOINT_UNREACHABLE;
				memset(pShortestPath, UCHAR_MAX, sizeof(unsigned short)*array_size);// faster

				for (index=0; index < route_num_waypoints; ++index)
					pShortestPath[index * route_num_waypoints + index] = 0;// zero diagonal

				for (row=0; row < route_num_waypoints; ++row)
				{
					if (paths[row] != NULL)
					{
						PATH *p = paths[row];
						while (p)
						{
							i = 0;
							while (i < MAX_PATH_INDEX)
							{
								if (p->index[i] != WAYPOINT_NONE)
								{
									index = p->index[i];
									// check if this is NOT team specific OR matches this team
									if (!(waypoints[index].flags & W_FL_TEAM_SPECIFIC) ||
										((waypoints[index].flags & W_FL_TEAM_BITMASK) == matrix))
									{
										distance = (waypoints[row].origin - waypoints[index].origin).Length();

										if (distance > (float)WAYPOINT_MAX_DISTANCE)
											distance = (float)WAYPOINT_MAX_DISTANCE;

										if (distance > REACHABLE_RANGE)
										{
											_snprintf(msg, 80, "Waypoint path distance > %4.1f at from %d to %d\n", REACHABLE_RANGE, row, index);
											conprintf(1, msg);
										}
										else
										{
											offset = row * route_num_waypoints + index;
											pShortestPath[offset] = (unsigned short)distance;
										}
									}
								}
								i++;
							}
							p = p->next;  // go to next node in linked list
						}
					}
				}

				// run Floyd's Algorithm to generate the from_to matrix...
				WaypointFloyds(pShortestPath, pFromTo);

				for (a=0; a < route_num_waypoints; ++a)
				{
					for (b=0; b < route_num_waypoints; ++b)
						if (pShortestPath[a * route_num_waypoints + b] == WAYPOINT_UNREACHABLE)
							pFromTo[a * route_num_waypoints + b] = WAYPOINT_UNREACHABLE;
				}

				bfp = fopen(filename2, "wb");
				if (bfp != NULL)
				{
					num_items = fwrite(shortest_path[matrix], sizeof(unsigned short), array_size, bfp);
					if (num_items != array_size)
					{
						// if couldn't write enough data, close file and delete it
						fclose(bfp);
						unlink(filename2);
					}
					else
					{
						num_items = fwrite(from_to[matrix], sizeof(unsigned short), array_size, bfp);
						fclose(bfp);
						if (num_items != array_size)
						{
							// if couldn't write enough data, delete file
							unlink(filename2);
						}
					}
					conprintf(1, "Waypoint path calculations for team %d complete!\n", matrix+1);
				}
				else
					conprintf(1, "Error writing waypoint paths!\n");
			}
		}
	}
}

void WaypointPrintSummary(void)
{
	conprintf(0, "route_num_waypoints %u, num_waypoints %u\n", route_num_waypoints, num_waypoints);
	//conprintf(0, "build_matrix: %d %d %d %d", build_matrix[]);
}
