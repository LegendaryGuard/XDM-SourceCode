#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "waypoint.h"
#include "bot.h"
#include "bot_cvar.h"
#include "bot_func.h"
#include "bot_weapons.h"
#include "pm_shared.h"// XDM3035c: VEC_HULL_*

// SET THIS UP BASED ON MOD!!!
int max_drop_height = 800;
static FILE *fp = NULL;

void BotFixIdealPitch(edict_t *pEdict)
{
	// check for wrap around of angle...
	if (pEdict->v.idealpitch >= 180.0f)
		pEdict->v.idealpitch -= 360 * ((int)(pEdict->v.idealpitch / 360.0f) + 1);
	else if (pEdict->v.idealpitch < -180.0f)
		pEdict->v.idealpitch += 360 * ((int)(-pEdict->v.idealpitch / 360.0f) + 1);
}

void BotFixIdealYaw(edict_t *pEdict)
{
	// check for wrap around of angle...
	if (pEdict->v.ideal_yaw >= 180.0f)
		pEdict->v.ideal_yaw -= 360 * ((int)(pEdict->v.ideal_yaw / 360.0f) + 1);
	else if (pEdict->v.ideal_yaw < -180.0f)
		pEdict->v.ideal_yaw += 360 * ((int)(-pEdict->v.ideal_yaw / 360.0f) + 1);
}

// returns the number of degrees left to turn toward ideal pitch...
float BotChangePitch(bot_t *pBot, float speed)
{
	float ideal;
	float current;
	float current_180;  // current +/- 180 degrees
	float diff;
	edict_t *pEdict = pBot->pEdict;

	// turn from the current v_angle pitch to the idealpitch by selecting
	// the quickest way to turn to face that direction
	current = pEdict->v.v_angle.x;
	ideal = pEdict->v.idealpitch;
	// find the difference in the current and ideal angle
	diff = fabs(current - ideal);
	speed = speed * pBot->f_frame_time;  // angles per second

	// check if difference is less than the max degrees per turn
	if (diff < speed)
		speed = diff;  // just need to turn a little bit (less than max)

	// here we have four cases, both angle positive, one positive and
	// the other negative, one negative and the other positive, or
	// both negative.  handle each case separately...

	if ((current >= 0) && (ideal >= 0))  // both positive
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}
	else if ((current >= 0) && (ideal < 0))
	{
		current_180 = current - 180.0f;
		
		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else if ((current < 0) && (ideal >= 0))
	{
		current_180 = current + 180.0f;
		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else  // (current < 0) && (ideal < 0)  both negative
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}

	// check for wrap around of angle...
	NormalizeAngle180(&current);

	pEdict->v.v_angle.x = current;
	return diff;  // return number of degrees left to turn
}

// returns the number of degrees left to turn toward ideal yaw...
float BotChangeYaw(bot_t *pBot, float speed)
{
	float ideal;
	float current;
	float current_180;  // current +/- 180 degrees
	float diff;
	edict_t *pEdict = pBot->pEdict;

	// turn from the current v_angle yaw to the ideal_yaw by selecting
	// the quickest way to turn to face that direction
	current = pEdict->v.v_angle.y;
	ideal = pEdict->v.ideal_yaw;
	
	// find the difference in the current and ideal angle
	diff = fabs(current - ideal);

	// speed that we can turn during this frame...
	speed = speed * pBot->f_frame_time;

	// check if difference is less than the max degrees per turn
	if (diff < speed)
		speed = diff;// just need to turn a little bit (less than max)

	// here we have four cases, both angle positive, one positive and
	// the other negative, one negative and the other positive, or
	// both negative.  handle each case separately...
	if ((current >= 0) && (ideal >= 0))  // both positive
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}
	else if ((current >= 0) && (ideal < 0))
	{
		current_180 = current - 180.0f;
		
		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else if ((current < 0) && (ideal >= 0))
	{
		current_180 = current + 180.0f;
		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else  // (current < 0) && (ideal < 0)  both negative
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}
	// check for wrap around of angle...
	NormalizeAngle180(&current);
	pEdict->v.v_angle.y = current;
	return diff;  // return number of degrees left to turn
}

void BotFixBodyAngles(edict_t *pEdict)// HPB40
{
	// check for wrap around of angle...
	if (pEdict->v.angles.x >= 180)
		pEdict->v.angles.x -= 360 * ((int)(pEdict->v.angles.x / 360.0f) + 1);
	else if (pEdict->v.angles.x < -180)
		pEdict->v.angles.x += 360 * ((int)(-pEdict->v.angles.x / 360.0f) + 1);

	if (pEdict->v.angles.y >= 180)
		pEdict->v.angles.y -= 360 * ((int)(pEdict->v.angles.y / 360.0f) + 1);
	else if (pEdict->v.angles.y < -180)
		pEdict->v.angles.y += 360 * ((int)(-pEdict->v.angles.y / 360.0f) + 1);

	if (pEdict->v.angles.z >= 180)
		pEdict->v.angles.z -= 360 * ((int)(pEdict->v.angles.z / 360.0f) + 1);
	else if (pEdict->v.angles.z < -180)
		pEdict->v.angles.z += 360 * ((int)(-pEdict->v.angles.z / 360.0f) + 1);
}

void BotFixViewAngles(edict_t *pEdict)// HPB40
{
	// check for wrap around of angle...
	if (pEdict->v.v_angle.x >= 180)
		pEdict->v.v_angle.x -= 360 * ((int) (pEdict->v.v_angle.x / 360.0f) + 1);
	else if (pEdict->v.v_angle.x < -180)
		pEdict->v.v_angle.x += 360 * ((int) (-pEdict->v.v_angle.x / 360.0f) + 1);

	if (pEdict->v.v_angle.y >= 180)
		pEdict->v.v_angle.y -= 360 * ((int) (pEdict->v.v_angle.y / 360.0f) + 1);
	else if (pEdict->v.v_angle.y < -180)
		pEdict->v.v_angle.y += 360 * ((int) (-pEdict->v.v_angle.y / 360.0f) + 1);

	if (pEdict->v.v_angle.z >= 180)
		pEdict->v.v_angle.z -= 360 * ((int) (pEdict->v.v_angle.z / 360.0f) + 1);
	else if (pEdict->v.v_angle.z < -180)
		pEdict->v.v_angle.z += 360 * ((int) (-pEdict->v.v_angle.z / 360.0f) + 1);
}

bool BotFindWaypoint( bot_t *pBot )
{
	DBG_BOT_PRINT("BotFindWaypoint(%s)\n", pBot->name);
	waypointindex_t index, select_index;
	waypointindex_t min_index[3];
	int path_index;
	float distance, min_distance[3];
	PATH *pPath = NULL;
	edict_t *pEdict = pBot->pEdict;

	/*for (index=0; index < 3; index++)
	{
		min_distance[index] = BOT_SEARCH_MAX_DISTANCE;
		min_index[index] = WAYPOINT_NONE;
	}*/
	min_distance[0] = BOT_SEARCH_MAX_DISTANCE;
	min_index[0] = WAYPOINT_NONE;
	min_distance[1] = BOT_SEARCH_MAX_DISTANCE;
	min_index[1] = WAYPOINT_NONE;
	min_distance[2] = BOT_SEARCH_MAX_DISTANCE;
	min_index[2] = WAYPOINT_NONE;

	index = WaypointFindPath(&pPath, &path_index, pBot->curr_waypoint_index, pEdict->v.team);

	while (index != WAYPOINT_NONE)
	{
		// if index is not a current or recent previous waypoint...
		if ((index != pBot->curr_waypoint_index) &&
			(index != pBot->prev_waypoint_index[0]) &&
			(index != pBot->prev_waypoint_index[1]) &&
			(index != pBot->prev_waypoint_index[2]) &&
			(index != pBot->prev_waypoint_index[3]) &&
			(index != pBot->prev_waypoint_index[4]))
		{
			// find the distance from the bot to this waypoint
			distance = (pEdict->v.origin - waypoints[index].origin).Length();
			if (distance < min_distance[0])
			{
				min_distance[2] = min_distance[1];
				min_index[2] = min_index[1];
				
				min_distance[1] = min_distance[0];
				min_index[1] = min_index[0];
				
				min_distance[0] = distance;
				min_index[0] = index;
			}
			else if (distance < min_distance [1])
			{
				min_distance[2] = min_distance[1];
				min_index[2] = min_index[1];
				
				min_distance[1] = distance;
				min_index[1] = index;
			}
			else if (distance < min_distance[2])
			{
				min_distance[2] = distance;
				min_index[2] = index;
			}
		}
		// find the next path to a waypoint
		index = WaypointFindPath(&pPath, &path_index, pBot->curr_waypoint_index, pEdict->v.team);
	}

	select_index = WAYPOINT_NONE;

	// about 20% of the time choose a waypoint at random
	// (don't do this any more often than every 10 seconds)
	if ((RANDOM_LONG(1, 100) <= 20) && (pBot->f_random_waypoint_time <= gpGlobals->time))
	{
		pBot->f_random_waypoint_time = gpGlobals->time + 10.0f;

		if (min_index[2] != WAYPOINT_NONE)
			index = RANDOM_LONG(0, 2);
		else if (min_index[1] != WAYPOINT_NONE)
			index = RANDOM_LONG(0, 1);
		else if (min_index[0] != WAYPOINT_NONE)
			index = 0;
		else
			return false;  // no waypoints found!

		select_index = min_index[index];
	}
	else
	{
		// use the closest waypoint that has been recently used
		select_index = min_index[0];
	}

	if (select_index != WAYPOINT_NONE)  // was a waypoint found?
	{
		pBot->prev_waypoint_index[4] = pBot->prev_waypoint_index[3];
		pBot->prev_waypoint_index[3] = pBot->prev_waypoint_index[2];
		pBot->prev_waypoint_index[2] = pBot->prev_waypoint_index[1];
		pBot->prev_waypoint_index[1] = pBot->prev_waypoint_index[0];
		pBot->prev_waypoint_index[0] = pBot->curr_waypoint_index;
		pBot->curr_waypoint_index = select_index;
		pBot->waypoint_origin = waypoints[select_index].origin;
		pBot->f_waypoint_time = gpGlobals->time;
		return true;
	}
	return false;  // couldn't find a waypoint
}


bool BotHeadTowardWaypoint(bot_t *pBot)
{
	DBG_BOT_PRINT("BotHeadTowardWaypoint(%s)\n", pBot->name);
	waypointindex_t i;
	waypointindex_t index;
	float waypoint_distance, min_distance;
	TEAM_ID team;
	//float pause_time = 0.0;
	Vector v_src, v_dest;
	TraceResult tr;
	bool touching;
	bool status;
	bool bot_has_flag = false;
	edict_t *pEdict = pBot->pEdict;

//	if (!checked_teamplay)  // check for team play...
//		BotCheckTeamplay();

	// is team play enabled (or is it Counter-Strike)?
	if (IsTeamplay())
		team = pEdict->v.team;//UTIL_GetTeam(pEdict);
	else
		team = TEAM_NONE;  // not team play (all waypoints are valid for everyone)

	// check if the bot has been trying to get to this waypoint for a while...
	if ((pBot->f_waypoint_time + 5.0) < gpGlobals->time)
	{
		pBot->curr_waypoint_index = WAYPOINT_NONE;  // forget about this waypoint
		pBot->waypoint_goal = WAYPOINT_NONE;  // also forget about a goal
	}

	// check if we need to find a waypoint...
	if (pBot->curr_waypoint_index == WAYPOINT_NONE)
	{
		pBot->waypoint_top_of_ladder = false;
		
		// did we just come off of a ladder or are we underwater?
		if (((pBot->f_end_use_ladder_time + 2.0f) > gpGlobals->time) || (pBot->pEdict->v.waterlevel == 3))
		{
			// find the nearest visible waypoint
			i = WaypointFindNearest(pEdict, REACHABLE_RANGE, team);
		}
		else
		{
			// find the nearest reachable waypoint
			i = WaypointFindReachable(pEdict, REACHABLE_RANGE, team);
		}

		if (i == WAYPOINT_NONE)
		{
			pBot->curr_waypoint_index = WAYPOINT_NONE;
			return false;
		}

		pBot->curr_waypoint_index = i;
		pBot->waypoint_origin = waypoints[i].origin;
		pBot->f_waypoint_time = gpGlobals->time;
	}
	else
	{
		// skip this part if bot is trying to get out of water...
		if (pBot->f_exit_water_time < gpGlobals->time)
		{
			// check if we can still see our target waypoint...
			v_src = pEdict->v.origin + pEdict->v.view_ofs;
			v_dest = waypoints[pBot->curr_waypoint_index].origin;

			// trace a line from bot's eyes to destination...
			UTIL_TraceLine(v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr);

			// check if line of sight to object is blocked (i.e. not visible)
			if (tr.flFraction < 1.0f)
			{
				// did we just come off of a ladder or are we under water?
				if (((pBot->f_end_use_ladder_time + 2.0f) > gpGlobals->time) || (pBot->pEdict->v.waterlevel == 3))
				{
					// find the nearest visible waypoint
					i = WaypointFindNearest(pEdict, REACHABLE_RANGE, team);
				}
				else
				{
					// find the nearest reachable waypoint
					i = WaypointFindReachable(pEdict, REACHABLE_RANGE, team);
				}

				if (i == WAYPOINT_NONE)
				{
					pBot->curr_waypoint_index = WAYPOINT_NONE;
					return false;
				}

				pBot->curr_waypoint_index = i;
				pBot->waypoint_origin = waypoints[i].origin;
				pBot->f_waypoint_time = gpGlobals->time;
			}
		}
	}
	
	// find the distance to the target waypoint
	waypoint_distance = (pEdict->v.origin - pBot->waypoint_origin).Length();

	// set the minimum distance from waypoint to be considered "touching" it
	min_distance = 50.0f;

	// if this is a crouch waypoint, bot must be fairly close...
	if (waypoints[pBot->curr_waypoint_index].flags & W_FL_CROUCH)
		min_distance = 20.0f;

	if (waypoints[pBot->curr_waypoint_index].flags & W_FL_JUMP)
		min_distance = 25.0f;

	if (waypoints[pBot->curr_waypoint_index].flags & W_FL_SENTRYGUN)
		min_distance = 20.0f;

	if (waypoints[pBot->curr_waypoint_index].flags & W_FL_DISPENSER)
		min_distance = 20.0f;

	// if this is a ladder waypoint, bot must be fairly close to get on ladder
	if (waypoints[pBot->curr_waypoint_index].flags & W_FL_LADDER)
		min_distance = 20.0f;
	
	// if trying to get out of water, need to get very close to waypoint...
	if (pBot->f_exit_water_time >= gpGlobals->time)
		min_distance = 20.0f;

	touching = false;

	// did the bot run past the waypoint? (prevent the loop-the-loop problem)
	if ((pBot->prev_waypoint_distance > 1.0f) && (waypoint_distance > pBot->prev_waypoint_distance))
		touching = true;

	// are we close enough to a target waypoint...
	if (waypoint_distance < min_distance)
		touching = true;

	// save current distance as previous
	pBot->prev_waypoint_distance = waypoint_distance;

	if (touching)
	{
		bool waypoint_found = false;
		pBot->prev_waypoint_distance = 0.0f;
		// check if the waypoint is a door waypoint
		if (waypoints[pBot->curr_waypoint_index].flags & W_FL_DOOR)
			pBot->f_dont_avoid_wall_time = gpGlobals->time + 5.0f;

		// check if the next waypoint is a jump waypoint...
		if (waypoints[pBot->curr_waypoint_index].flags & W_FL_JUMP)
			pEdict->v.button |= IN_JUMP;  // jump here

		// check if the waypoint is a sniper waypoint...
		if (waypoints[pBot->curr_waypoint_index].flags & W_FL_SNIPER)
		{
			waypointindex_t aim_index = WaypointFindNearestAiming(waypoints[pBot->curr_waypoint_index].origin);
			if (aim_index != WAYPOINT_NONE)
			{
				Vector v_aim = waypoints[aim_index].origin - waypoints[pBot->curr_waypoint_index].origin;
				//Vector aim_angles = UTIL_VecToAngles( v_aim );
				//pEdict->v.ideal_yaw = aim_angles.y;
				pEdict->v.ideal_yaw = VecToYaw(v_aim);
				BotFixIdealYaw(pEdict);
			}
			pBot->f_pause_time = gpGlobals->time + RANDOM_FLOAT(20, 30);
			// fix f_waypoint_time so bot won't think it is stuck
			pBot->f_waypoint_time = pBot->f_pause_time;
			return true;
		}

		// check if the bot has reached the goal waypoint...
		if (pBot->curr_waypoint_index == pBot->waypoint_goal)
		{
			pBot->waypoint_goal = WAYPOINT_NONE;// forget this goal waypoint
			if (pBot->waypoint_near_flag)
			{
				pBot->waypoint_near_flag = false;
				// just head towards the flag/card/ball
				//Vector v_flag = pBot->waypoint_flag_origin - pEdict->v.origin;
				///Vector bot_angles = UTIL_VecToAngles( v_flag );
				///pEdict->v.ideal_yaw = bot_angles.y;
				//pEdict->v.ideal_yaw = VecToYaw(v_flag);
				//BotFixIdealYaw(pEdict);
				return true;
			}
		}

		// check if the bot is carrying the flag/card/ball...
		if (bot_has_flag)
		{
			// find the nearest flag goal waypoint...
			index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index, team, W_FL_FLAG_GOAL);
			pBot->waypoint_goal = index;  // goal index or WAYPOINT_NONE
			pBot->waypoint_near_flag = false;
		}
		else
			pBot->bot_has_flag = false;

		// test special case of bot underwater and close to surface...
		if (pBot->pEdict->v.waterlevel == 3)
		{
			//Vector v_src, v_dest;
			//TraceResult tr;
			int contents;

			// trace a line straight up 100 units...
			v_src = pEdict->v.origin;
			v_dest = v_src;
			v_dest.z += 100.0f;

			// trace a line to destination...
			UTIL_TraceLine(v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr);

			if (tr.flFraction >= 1.0)
			{
				// find out what the contents is of the end of the trace...
				contents = POINT_CONTENTS( tr.vecEndPos );
				// check if the trace endpoint is in open space...
				if (contents == CONTENTS_EMPTY)
				{
					// find the nearest visible waypoint
					i = WaypointFindNearest(tr.vecEndPos, pEdict, 100.0f, team);
					if (i != WAYPOINT_NONE)
					{
						waypoint_found = true;
						pBot->curr_waypoint_index = i;
						pBot->waypoint_origin = waypoints[i].origin;
						pBot->f_waypoint_time = gpGlobals->time;
						// keep trying to exit water for next 3 seconds
						pBot->f_exit_water_time = gpGlobals->time + 3.0f;
					}
				}
			}
		}

		// if the bot doesn't have a goal waypoint then pick one...
		if ((pBot->waypoint_goal == WAYPOINT_NONE) && (pBot->f_waypoint_goal_time < gpGlobals->time))
		{
			// don't pick a goal more often than every 10 seconds...
			pBot->f_waypoint_goal_time = gpGlobals->time + 10.0f;
			pBot->waypoint_near_flag = false;
			// IF HEALTH LESS THAN CRITICAL LEVEL, GO FIND HEALTH!!!
			// IF AMMO LESS THAN CRITICAL LEVEL, GO FIND AMMO!!!
			// GO FIND WEAPONS HERE!!!
			if (RANDOM_LONG(1, 100) <= 50)
				index = WaypointFindNearestGoal(pEdict, pBot->curr_waypoint_index, team, W_FL_WEAPON, pBot->weapon_points);
			else
			{
				size_t count = 0;
				index = WAYPOINT_NONE;
				while ((index == WAYPOINT_NONE) && (count < 3))
				{
					index = WaypointFindRandomGoal(pEdict, team, W_FL_WEAPON, pBot->weapon_points);
					count++;
				}
			}

			if (index != WAYPOINT_NONE)
			{
				pBot->waypoint_goal = index;
				pBot->weapon_points[4] = pBot->weapon_points[3];
				pBot->weapon_points[3] = pBot->weapon_points[2];
				pBot->weapon_points[2] = pBot->weapon_points[1];
				pBot->weapon_points[1] = pBot->weapon_points[0];
				pBot->weapon_points[0] = pBot->waypoint_goal;
			}
		}

		// check if the bot has a goal waypoint...
		if (pBot->waypoint_goal != WAYPOINT_NONE)
		{
			// get the next waypoint to reach goal...
			i = WaypointRouteFromTo(pBot->curr_waypoint_index, pBot->waypoint_goal, team, false);
			if (i != WAYPOINT_UNREACHABLE)  // can we get to the goal from here?
			{
				waypoint_found = true;
				pBot->curr_waypoint_index = i;
				pBot->waypoint_origin = waypoints[i].origin;
				pBot->f_waypoint_time = gpGlobals->time;
			}
		}

		if (waypoint_found == false)
		{
			index = 4;
			// try to find the next waypoint
			while (((status = BotFindWaypoint(pBot)) == false) && (index > 0))
			{
				// if waypoint not found, clear oldest prevous index and try again
				pBot->prev_waypoint_index[index] = WAYPOINT_NONE;
				index--;
			}
			if (status == false)
			{
				pBot->curr_waypoint_index = WAYPOINT_NONE;  // indicate no waypoint found
				// clear all previous waypoints...
				for (index=0; index < 5; ++index)
					pBot->prev_waypoint_index[index] = WAYPOINT_NONE;
				
				return false;
			}
		}
	}
	// check if the next waypoint is on a ladder AND
	// the bot it not currently on a ladder...
	if ((waypoints[pBot->curr_waypoint_index].flags & W_FL_LADDER) && !IsOnLadder(pEdict))
	{
		// if it's origin is lower than the bot...
		if (waypoints[pBot->curr_waypoint_index].origin.z < pEdict->v.origin.z)
			pBot->waypoint_top_of_ladder = true;
	}
	else
		pBot->waypoint_top_of_ladder = false;

	// keep turning towards the waypoint...
	Vector v_direction(pBot->waypoint_origin); v_direction -= pEdict->v.origin;
	Vector v_angles;// = UTIL_VecToAngles(v_direction);
	VectorAngles(v_direction, v_angles);
	// if the bot is NOT on a ladder, change the yaw...
	if (!IsOnLadder(pEdict))
	{
#if !defined (NOSQB)
		pEdict->v.idealpitch = -v_angles.x;// TODO: TESTME: SQB?
#endif
		BotFixIdealPitch(pEdict);
		pEdict->v.ideal_yaw = v_angles.y;
		BotFixIdealYaw(pEdict);
	}
	return true;
}

void BotOnLadder(bot_t *pBot, const vec_t &moved_distance)
{
	DBG_BOT_PRINT("BotOnLadder(%s)\n", pBot->name);
	float angle = 0.0f;
	Vector v_src, v_dest, view_angles;
	edict_t *pEdict = pBot->pEdict;
	TraceResult tr;
	bool done = false;

	// check if the bot has JUST touched this ladder...
	if (pBot->ladder_dir == LADDER_UNKNOWN)
	{
		// try to square up the bot on the ladder...
		while ((!done) && (angle < 180.0f))
		{
			// try looking in one direction (forward + angle)
			view_angles = pEdict->v.v_angle;
			view_angles.y = pEdict->v.v_angle.y + angle;
			NormalizeAngle360(&view_angles.y);// XDM3038a

			UTIL_MakeVectors(view_angles);
			v_src = pEdict->v.origin + pEdict->v.view_ofs;
			v_dest = v_src + gpGlobals->v_forward * 30.0f;
			UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

			if (tr.flFraction < 1.0)  // hit something?
			{
				if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
				{
					// square up to the wall...
					/*view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);
					// Normal comes OUT from wall, so flip it around...
					view_angles.y += 180;

					if (view_angles.y > 180)
						view_angles.y -= 360;

					pEdict->v.ideal_yaw = view_angles.y;*/
					pEdict->v.ideal_yaw = VecToYaw(tr.vecPlaneNormal);
					pEdict->v.ideal_yaw += 180;
					NormalizeAngle180(&pEdict->v.ideal_yaw);
					BotFixIdealYaw(pEdict);
					done = true;
				}
			}
			else
			{
				// try looking in the other direction (forward - angle)
				view_angles = pEdict->v.v_angle;
				view_angles.y = pEdict->v.v_angle.y - angle;
				NormalizeAngle360(&view_angles.y);// XDM3038a
				UTIL_MakeVectors(view_angles);
				v_src = pEdict->v.origin + pEdict->v.view_ofs;
				v_dest = v_src + gpGlobals->v_forward * 30.0f;
				UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

				if (tr.flFraction < 1.0)  // hit something?
				{
					if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
					{
						// square up to the wall...
						/*view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);
						// Normal comes OUT from wall, so flip it around...
						view_angles.y += 180;
						if (view_angles.y > 180)
							view_angles.y -= 360;
						
						pEdict->v.ideal_yaw = view_angles.y;*/
						pEdict->v.ideal_yaw = VecToYaw(tr.vecPlaneNormal) + 180.0f;
						NormalizeAngle180(&pEdict->v.ideal_yaw);
						BotFixIdealYaw(pEdict);
						done = true;
					}
				}
			}
			angle += 10;
		}

		if (!done)  // if didn't find a wall, just reset ideal_yaw...
		{
			// set ideal_yaw to current yaw (so bot won't keep turning)
			pEdict->v.ideal_yaw = pEdict->v.v_angle.y;
			BotFixIdealYaw(pEdict);
		}
	}
	// moves the bot up or down a ladder.  if the bot can't move
	// (i.e. get's stuck with someone else on ladder), the bot will
	// change directions and go the other way on the ladder.
	if (pBot->ladder_dir == LADDER_UP)  // is the bot currently going up?
	{
		pEdict->v.v_angle.x = PITCH_LOOK_UPWD;  // look upwards
		// check if the bot hasn't moved much since the last location...
		if ((moved_distance <= 1) && (pBot->f_prev_speed >= 1.0f))
		{
			// the bot must be stuck, change directions...
			pEdict->v.v_angle.x = PITCH_LOOK_DNWD;  // look downwards
			pBot->ladder_dir = LADDER_DOWN;
		}
	}
	else if (pBot->ladder_dir == LADDER_DOWN)  // is the bot currently going down?
	{
		pEdict->v.v_angle.x = PITCH_LOOK_DNWD;  // look downwards
		
		// check if the bot hasn't moved much since the last location...
		if ((moved_distance <= 1) && (pBot->f_prev_speed >= 1.0f))
		{
			// the bot must be stuck, change directions...
			pEdict->v.v_angle.x = PITCH_LOOK_UPWD;  // look upwards
			pBot->ladder_dir = LADDER_UP;
		}
	}
	else  // the bot hasn't picked a direction yet, try going up...
	{
		pEdict->v.v_angle.x = PITCH_LOOK_UPWD;  // look upwards
		pBot->ladder_dir = LADDER_UP;
	}
	// move forward (i.e. in the direction the bot is looking, up or down)
	pEdict->v.button |= IN_FORWARD;
}


void BotUnderWater(bot_t *pBot)
{
	DBG_BOT_PRINT("BotUnderWater(%s)\n", pBot->name);
	bool found_waypoint = false;
	edict_t *pEdict = pBot->pEdict;
	
	// are there waypoints in this level (and not trying to exit water)?
	if ((num_waypoints > 0) && (pBot->f_exit_water_time < gpGlobals->time))
	{
		// head towards a waypoint
		found_waypoint = BotHeadTowardWaypoint(pBot);
	}
	
	if (found_waypoint == false)
	{
		// handle movements under water.  right now, just try to keep from
		// drowning by swimming up towards the surface and look to see if
		// there is a surface the bot can jump up onto to get out of the
		// water.  bots DON'T like water!
		Vector v_src, v_forward;
		TraceResult tr;
		int contents;
		// swim up towards the surface
		pEdict->v.v_angle.x = PITCH_LOOK_UPWD;  // look upwards
		// move forward (i.e. in the direction the bot is looking, up or down)
		pEdict->v.button |= IN_FORWARD;
		// set gpGlobals angles based on current view angle (for TraceLine)
		AngleVectors(pEdict->v.v_angle, v_forward, NULL, NULL);//UTIL_MakeVectors(pEdict->v.v_angle);
		// look from eye position straight forward (remember: the bot is looking
		// upwards at a 60 degree angle so TraceLine will go out and up...
		v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
		v_forward *= 90.0f; v_forward += v_src;//v_forward = v_src + gpGlobals->v_forward * 90;
		// trace from the bot's eyes straight forward...
		UTIL_TraceLine(v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
		// check if the trace didn't hit anything (i.e. nothing in the way)...
		if (tr.flFraction >= 1.0)
		{
			// find out what the contents is of the end of the trace...
			contents = POINT_CONTENTS( tr.vecEndPos );
			// check if the trace endpoint is in open space...
			if (contents == CONTENTS_EMPTY)
			{
				// ok so far, we are at the surface of the water, continue...
				v_src = tr.vecEndPos;
				v_forward = v_src;
				v_forward.z -= 90.0f;
				// trace from the previous end point straight down...
				UTIL_TraceLine(v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
				// check if the trace hit something...
				if (tr.flFraction < 1.0)
				{
					contents = POINT_CONTENTS(tr.vecEndPos);
					// if contents isn't water then assume it's land, jump!
					if (contents != CONTENTS_WATER)
						pEdict->v.button |= IN_JUMP;
				}
			}
		}
	}
}

void BotUseLift(bot_t *pBot, const vec_t &moved_distance)
{
	DBG_BOT_PRINT("BotUseLift(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	// just need to press the button once, when the flag gets set...
	if (pBot->f_use_button_time == gpGlobals->time)
	{
		pEdict->v.button = IN_USE;
		// face opposite from the button
		pEdict->v.ideal_yaw += 180.0f;  // rotate 180 degrees
		BotFixIdealYaw(pEdict);
	}
	// check if the bot has waited too long for the lift to move...
	if (((pBot->f_use_button_time + 2.0f) < gpGlobals->time) && (!pBot->b_lift_moving))
	{
		// clear use button flag
		pBot->b_use_button = false;
		// bot doesn't have to set f_find_item since the bot
		// should already be facing away from the button
		pBot->f_move_speed = pBot->f_max_speed;
	}
	// check if lift has started moving...
	if ((moved_distance > 1) && (!pBot->b_lift_moving))
		pBot->b_lift_moving = true;
	
	// check if lift has stopped moving...
	if ((moved_distance <= 1) && (pBot->b_lift_moving))
	{
		TraceResult tr1, tr2;
		Vector v_src, v_forward, v_right, v_left;
		Vector v_down, v_forward_down, v_right_down, v_left_down;
		pBot->b_use_button = false;

		// TraceLines in 4 directions to find which way to go...
		UTIL_MakeVectors(pEdict->v.v_angle);

		v_src = pEdict->v.origin + pEdict->v.view_ofs;
		v_forward = v_src + gpGlobals->v_forward * 90.0f;
		v_right = v_src + gpGlobals->v_right * 90.0f;
		v_left = v_src + gpGlobals->v_right * -90.0f;
		v_down = pEdict->v.v_angle;
		v_down.x += 45.0f;// look down at 45 degree angle

		UTIL_MakeVectors(v_down);

		v_forward_down = v_src + gpGlobals->v_forward * 100.0f;
		v_right_down = v_src + gpGlobals->v_right * 100.0f;
		v_left_down = v_src + gpGlobals->v_right * -100.0f;

		// try tracing forward first...
		UTIL_TraceLine(v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr1);
		UTIL_TraceLine(v_src, v_forward_down, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr2);

		// check if we hit a wall or didn't find a floor...
		if ((tr1.flFraction < 1.0f) || (tr2.flFraction >= 1.0f))
		{
			// try tracing to the RIGHT side next...
			UTIL_TraceLine(v_src, v_right, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr1);
			UTIL_TraceLine(v_src, v_right_down, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr2);

			// check if we hit a wall or didn't find a floor...
			if ((tr1.flFraction < 1.0f) || (tr2.flFraction >= 1.0f))
			{
				// try tracing to the LEFT side next...
				UTIL_TraceLine(v_src, v_left, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr1);
				UTIL_TraceLine(v_src, v_left_down, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr2);
				// check if we hit a wall or didn't find a floor...
				if ((tr1.flFraction < 1.0f) || (tr2.flFraction >= 1.0f))// only thing to do is turn around...
					pEdict->v.ideal_yaw += 180.0f;// turn all the way around
				else
					pEdict->v.ideal_yaw += 90.0f;// turn to the LEFT
			}
			else
				pEdict->v.ideal_yaw -= 90.0f;// turn to the RIGHT

			BotFixIdealYaw(pEdict);
		}
		BotChangeYaw(pBot, pEdict->v.yaw_speed);
		pBot->f_move_speed = pBot->f_max_speed;
	}
}

bool BotStuckInCorner(bot_t *pBot)
{
	DBG_BOT_PRINT("BotStuckInCorner(%s)\n", pBot->name);
	TraceResult tr;
	Vector v_src, v_dest;
	edict_t *pEdict = pBot->pEdict;

	UTIL_MakeVectors(pEdict->v.v_angle);

	// trace 45 degrees to the right...
	v_src = pEdict->v.origin;
	v_dest = v_src + gpGlobals->v_forward*20.0f + gpGlobals->v_right*20.0f;
	UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pEdict/* XDM3038: TESTME ->v.pContainingEntity*/, &tr);

	if (tr.flFraction >= 1.0f)
		return false;  // no wall, so not in a corner

	// trace 45 degrees to the left...
//already	v_src = pEdict->v.origin;
	v_dest = v_src + gpGlobals->v_forward*20 - gpGlobals->v_right*20;
	UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pEdict/* XDM3038: TESTME ->v.pContainingEntity*/, &tr);
	if (tr.flFraction >= 1.0f)
		return false;  // no wall, so not in a corner

	DBG_BOT_PRINT("BotStuckInCorner(%s): yes\n", pBot->name);
	return true;  // bot is in a corner
}

void BotTurnAtWall(bot_t *pBot, TraceResult *tr, bool negative)
{
	DBG_BOT_PRINT("BotTurnAtWall(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	Vector Normal;
	float Y, Y1, Y2, D1, D2, Z;

	// Find the normal vector from the trace result.  The normal vector will
	// be a vector that is perpendicular to the surface from the TraceResult.
//	Normal = UTIL_VecToAngles(tr->vecPlaneNormal);
	VectorAngles(tr->vecPlaneNormal, Normal);// XDM3038a

	// Since the bot keeps it's view angle in -180 < x < 180 degrees format,
	// and since TraceResults are 0 < x < 360, we convert the bot's view
	// angle (yaw) to the same format at TraceResult.
	Y = pEdict->v.v_angle.y + 180.0f;
	if (Y > 359.0f) Y -= 360.0f;

	if (negative)
	{
		// Turn the normal vector around 180 degrees (i.e. make it point towards
		// the wall not away from it.  That makes finding the angles that the
		// bot needs to turn a little easier.
		Normal.y -= 180.0f;
	}

	if (Normal.y < 0.0f)
		Normal.y += 360.0f;

	// Here we compare the bots view angle (Y) to the Normal - 90 degrees (Y1)
	// and the Normal + 90 degrees (Y2).  These two angles (Y1 & Y2) represent
	// angles that are parallel to the wall surface, but heading in opposite
	// directions.  We want the bot to choose the one that will require the
	// least amount of turning (saves time) and have the bot head off in that
	// direction.
	Y1 = Normal.y - 90.0f;
	if (RANDOM_LONG(1, 100) <= 50)
		Y1 = Y1 - RANDOM_FLOAT(5.0f, 20.0f);

	if (Y1 < 0.0f) Y1 += 360.0f;

	Y2 = Normal.y + 90.0f;
	if (RANDOM_LONG(1, 100) <= 50)
		Y2 = Y2 + RANDOM_FLOAT(5.0, 20.0);

	if (Y2 > 359) Y2 -= 360;

	// D1 and D2 are the difference (in degrees) between the bot's current
	// angle and Y1 or Y2 (respectively).
	D1 = fabs(Y - Y1);
	if (D1 > 179.0f) D1 = fabs(D1 - 360.0f);
	D2 = fabs(Y - Y2);
	if (D2 > 179.0f) D2 = fabs(D2 - 360.0f);
	
	// If difference 1 (D1) is more than difference 2 (D2) then the bot will
	// have to turn LESS if it heads in direction Y1 otherwise, head in
	// direction Y2.  I know this seems backwards, but try some sample angles
	// out on some graph paper and go through these equations using a
	// calculator, you'll see what I mean.
	if (D1 > D2)
		Z = Y1;
	else
		Z = Y2;

	// convert from TraceResult 0 to 360 degree format back to bot's -180 to 180 degree format.
	if (Z > 180.0f)
		Z -= 360.0f;

	// set the direction to head off into...
	pEdict->v.ideal_yaw = Z;
	BotFixIdealYaw(pEdict);
}


bool BotCantMoveForward(bot_t *pBot, TraceResult *tr)
{
	DBG_BOT_PRINT("BotCantMoveForward(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	// use some TraceLines to determine if anything is blocking the current path of the bot.
	Vector v_src, v_forward;
	UTIL_MakeVectors( pEdict->v.v_angle );
	// first do a trace from the bot's eyes forward...

	v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
	v_forward = v_src + gpGlobals->v_forward * 40.0f;

	// trace from the bot's eyes straight forward...
	UTIL_TraceLine(v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f)
		return true;  // bot's head will hit something

	// bot's head is clear, check at waist level...
	v_src = pEdict->v.origin;
	v_forward = v_src + gpGlobals->v_forward * 40.0f;
	// trace from the bot's waist straight forward...
	UTIL_TraceLine(v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, tr);

	// check if the trace hit something...
	if (tr->flFraction < 1.0f)
		return true;  // bot's body will hit something

	DBG_BOT_PRINT("BotCantMoveForward(%s): no\n", pBot->name);
	return false;  // bot can move forward, return false
}

// What I do here is trace 3 lines straight out, one unit higher than
// the highest normal jumping distance.  I trace once at the center of
// the body, once at the right side, and once at the left side.  If all
// three of these TraceLines don't hit an obstruction then I know the
// area to jump to is clear.  I then need to trace from head level,
// above where the bot will jump to, downward to see if there is anything
// blocking the jump.  There could be a narrow opening that the body
// will not fit into.  These horizontal and vertical TraceLines seem
// to catch most of the problems with falsely trying to jump on something
// that the bot can not get onto.
bool BotCanJumpUp(bot_t *pBot, bool *bDuckJump)
{
	DBG_BOT_PRINT("BotCanJumpUp(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	Vector v_jump, v_source, v_dest;
	TraceResult tr;
	bool check_duck = false;

	*bDuckJump = false;

	// convert current view angle to vectors for TraceLine math...
	v_jump = pEdict->v.v_angle;
	v_jump.x = 0;  // reset pitch to 0 (level horizontally)
	v_jump.z = 0;  // reset roll to 0 (straight up and down)

	UTIL_MakeVectors( v_jump );

	// use center of the body first...

	// maximum normal jump height is 45, so check one unit above that (46)
	v_source = pEdict->v.origin + Vector(0, 0, -36 + 46);
	v_dest = v_source + gpGlobals->v_forward * 24.0f;

	// trace a line forward at maximum jump height...
	UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// if trace hit something, check duck jump...
	if (tr.flFraction < 1.0)
		check_duck = true;

	if (!check_duck)
	{
		// now check same height to one side of the bot...
		v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 46);
		v_dest = v_source + gpGlobals->v_forward * 24.0f;
		// trace a line forward at maximum jump height...
		UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
		// if trace hit something, check duck jump...
		if (tr.flFraction < 1.0)
			check_duck = true;
	}

	if (!check_duck)
	{
		// now check same height on the other side of the bot...
		v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 46);
		v_dest = v_source + gpGlobals->v_forward * 24.0f;
		// trace a line forward at maximum jump height...
		UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
		// if trace hit something, check duck jump...
		if (tr.flFraction < 1.0)
			check_duck = true;
	}

	if (check_duck)
	{
		// maximum crouch jump height is 63, so check one unit above that (64)
		v_source = pEdict->v.origin + Vector(0, 0, -36 + 64);
		v_dest = v_source + gpGlobals->v_forward * 24.0f;

		// trace a line forward at maximum jump height...
		UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

		// if trace hit something, return FALSE
		if (tr.flFraction < 1.0)
			return false;

		// now check same height on the other side of the bot...
		v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 64);
		v_dest = v_source + gpGlobals->v_forward * 24.0f;

		// trace a line forward at maximum jump height...
		UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

		// if trace hit something, return FALSE
		if (tr.flFraction < 1.0)
			return false;

		// now check same height on the other side of the bot...
		v_source = pEdict->v.origin + gpGlobals->v_right * -16.0f + Vector(0, 0, -36 + 64);
		v_dest = v_source + gpGlobals->v_forward * 24.0f;

		// trace a line forward at maximum jump height...
		UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

		// if trace hit something, return FALSE
		if (tr.flFraction < 1.0)
			return false;
	}

	// now trace from head level downward to check for obstructions...

	// start of trace is 24 units in front of bot...
	v_source = pEdict->v.origin + gpGlobals->v_forward * 24.0f;

	if (check_duck)
		// offset 36 units if crouch-jump (36 + 36)
		v_source.z += 72.0f;
	else
		// offset 72 units from top of head (72 + 36)
		v_source.z += 108.0f;

	if (check_duck)
		// end point of trace is 27 units straight down from start...
		// (27 is 72 minus the jump limit height which is 63 - 18 = 45)
		v_dest = v_source + Vector(0, 0, -27);
	else
		// end point of trace is 99 units straight down from start...
		// (99 is 108 minus the jump limit height which is 45 - 36 = 9)
		v_dest = v_source + Vector(0, 0, -99);

	// trace a line straight down toward the ground...
	UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height to one side of the bot...
	v_source = pEdict->v.origin + gpGlobals->v_right * 16.0f + gpGlobals->v_forward * 24.0f;

	if (check_duck)
		v_source.z += 72.0f;
	else
		v_source.z += 108.0f;

	if (check_duck)
		v_dest = v_source + Vector(0, 0, -27);
	else
		v_dest = v_source + Vector(0, 0, -99);

	// trace a line straight down toward the ground...
	UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0f)
		return false;

	// now check same height on the other side of the bot...
	v_source = pEdict->v.origin + gpGlobals->v_right * -16.0f + gpGlobals->v_forward * 24.0f;

	if (check_duck)
		v_source.z += 72.0f;
	else
		v_source.z += 108.0f;

	if (check_duck)
		v_dest = v_source + Vector(0, 0, -27);
	else
		v_dest = v_source + Vector(0, 0, -99);

	// trace a line straight down toward the ground...
	UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return false;

	DBG_BOT_PRINT("BotCanJumpUp(%s): yes\n", pBot->name);
	return true;
}

// UNDONE
bool PointFitBox(const Vector &vecPoint, const Vector &vecMinS, const Vector &vecMaxS, const Vector &vecFw, const Vector &vecRt)
{
	return false;
}

bool BotCanDuckUnder(bot_t *pBot)
{
	DBG_BOT_PRINT("BotCanDuckUnder(%s)\n", pBot->name);
	// What I do here is trace 3 lines straight out, one unit higher than
	// the ducking height.  I trace once at the center of the body, once
	// at the right side, and once at the left side.  If all three of these
	// TraceLines don't hit an obstruction then I know the area to duck to
	// is clear.  I then need to trace from the ground up, 72 units, to make
	// sure that there is something blocking the TraceLine.  Then we know
	// we can duck under it.

	// XDM3035c: new algorithm
	// trace from player center forward,
	// then trace fro that end point to each of 4 sides to check if there's enough space to fit
	// IDEA: we should really use the BSP tree sometimes, if not always...
	TraceResult tr;
	Vector v_duck, v_source, v_dest;
	edict_t *pEdict = pBot->pEdict;

	// convert current view angle to vectors for TraceLine math...
	v_duck = pEdict->v.v_angle;
	v_duck.x = 0;  // reset pitch to 0 (level horizontally)
	v_duck.z = 0;  // reset roll to 0 (straight up and down)

	UTIL_MakeVectors(v_duck);

	// use center of the body first...

	// duck height is 36, so check one unit above that (37)
	v_source = pEdict->v.origin + Vector(0,0,DUCK_VIEW);// origin is bottom!
	v_dest = v_source + gpGlobals->v_forward * 24;

	// trace a line forward at duck height...
	UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, dont_ignore_glass, pEdict->v.pContainingEntity, &tr);

//	UTIL_DebugBeam(v_source, tr.vecEndPos, 3.0f, 255,255,255);
	// if trace hit something, return FALSE
	if (tr.flFraction < 1.0)
		return false;

	v_source = tr.vecEndPos;// Now we're in the hole, trace to its sides!
//	UTIL_DebugPoint(v_source, 3.0f, 255,255,0);

	v_dest = v_source + gpGlobals->v_right * HULL_RADIUS*2;// WARNING! no normal angle correction is done, and v_right is relative to PLAYER, not to any wall surface!
//	UTIL_DebugBeam(v_source, v_dest, 3.0f, 255,255,0);
	UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, dont_ignore_glass, pEdict->v.pContainingEntity, &tr);

	vec_t flDistance = (tr.vecEndPos - v_source).Length();

	color24 c = {255,255,0};
	if (tr.flFraction < 1.0)// if we hit wall on the right, trace from hit point to the left using the normal
	{
		c.g = 0;
		v_source = tr.vecEndPos;
		v_dest = v_source + tr.vecPlaneNormal * -HULL_RADIUS*2;// same here
	}
	else// just trace from previous center to our left
		v_dest = v_source + gpGlobals->v_right * -HULL_RADIUS*2;// same here

	UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, dont_ignore_glass, pEdict->v.pContainingEntity, &tr);
//	UTIL_DebugBeam(v_source, v_dest, 3.0f, c.r, c.g, c.b);
	flDistance += (tr.vecEndPos - v_source).Length();
	if (flDistance < (HULL_RADIUS*2 - 1))// not wide enough to fit // trace will add up to 31.999
		return false;

	// now trace from the ground up to check for object to duck under...

	// start of trace is 24 units in front of bot near ground...
/*	v_source = pEdict->v.origin + gpGlobals->v_forward * 24;
	v_source.z = v_source.z - 35;  // offset to feet + 1 unit up

	// end point of trace is 72 units straight up from start...
	v_dest = v_source + Vector(0, 0, 72);

	// trace a line straight up in the air...
	UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// if trace didn't hit something, return FALSE
	if (tr.flFraction >= 1.0)
		return false;

	// now check same height to one side of the bot...
	v_source = pEdict->v.origin + gpGlobals->v_right * HULL_RADIUS + gpGlobals->v_forward * 24;
	v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
	v_dest = v_source + Vector(0, 0, 72);

	// trace a line straight up in the air...
	UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// if trace didn't hit something, return FALSE
	if (tr.flFraction >= 1.0)
		return false;

	// now check same height on the other side of the bot...
	v_source = pEdict->v.origin + gpGlobals->v_right * -HULL_RADIUS + gpGlobals->v_forward * 24;
	v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
	v_dest = v_source + Vector(0, 0, 72);

	// trace a line straight up in the air...
	UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// if trace didn't hit something, return FALSE
	if (tr.flFraction >= 1.0)
		return false;
*/
	DBG_BOT_PRINT("BotCanDuckUnder(%s): yes\n", pBot->name);
	return true;
}

void BotRandomTurn(bot_t *pBot)
{
	pBot->f_move_speed = 0;  // don't move while turning
	if (RANDOM_LONG(1, 100) <= 10)
	{
		// 10 percent of the time turn completely around...
		pBot->pEdict->v.ideal_yaw += 180;
	}
	else
	{
		// turn randomly between 30 and 60 degress
		if (pBot->wander_dir == WANDER_LEFT)
			pBot->pEdict->v.ideal_yaw += RANDOM_LONG(30, 60);
		else
			pBot->pEdict->v.ideal_yaw -= RANDOM_LONG(30, 60);
	}
	BotFixIdealYaw(pBot->pEdict);
}

bool BotFollowUser(bot_t *pBot)
{
	DBG_BOT_PRINT("BotFollowUser(%s)\n", pBot->name);
	// the bot's user is dead!
	if (!IsAlive(pBot->pBotUser))
	{
		DBG_BOT_PRINT("BotFollowUser(%s): user is dead\n", pBot->name);
		//BotSpeakDirect(pBot, "Noooooooooo!", 0.0);
		return false;
	}
	edict_t *pEdict = pBot->pEdict;

	if (IsTeamplay() && pEdict->v.team != pBot->pBotUser->v.team)// XDM3038c: prevent cheaters from using bots and changing teams
		return false;

	Vector vecEnd = pBot->pBotUser->v.origin + pBot->pBotUser->v.view_ofs;
	bool user_visible = FInViewCone(vecEnd, pEdict) && FVisible(vecEnd, pEdict);
	// check if the "user" is still visible or if the user has been visible
	// in the last 5 seconds (or the player just starting "using" the bot)
	if (user_visible || (pBot->f_bot_use_time + 5 > gpGlobals->time))
	{
		if (user_visible)
			pBot->f_bot_use_time = gpGlobals->time;// reset "last visible time"

		Vector v_user(vecEnd); v_user -= (pEdict->v.origin + pEdict->v.view_ofs);// face the user
		/*if(pBot->b_follow_look)
		{
			pEdict->v.ideal_yaw = RANDOM_LONG(60, 160) * RANDOM_LONG(-1,1);
			pBot->b_follow_look = FALSE;
		}
		else
		{*/
			//Vector bot_angles = UTIL_VecToAngles(v_user);
			Vector bot_angles;
			VectorAngles(v_user, bot_angles);// XDM3038a
//#if !defined (NOSQB)
//			bot_angles.x = -bot_angles.x;// SQB
//#endif
			pEdict->v.ideal_yaw = bot_angles.y;
		//}

		BotFixIdealYaw(pEdict);
		pEdict->v.idealpitch = bot_angles.x;
		BotFixIdealPitch(pEdict);

		vec_t f_distance = v_user.Length();// how far away is the "user"?
		//if (f_distance > g_bot_follow_distance.value*6.0f)
		//	BotSpeak("Hey! Wait for me!")

		if (f_distance > g_bot_follow_distance.value*2.0f)// run if distance to enemy is far
		{
			pBot->f_move_speed = pBot->f_max_speed;
		}
		else
		{
			if (f_distance > g_bot_follow_distance.value)// walk if distance is closer
			{
				pBot->f_move_speed = 0.5f*pBot->f_max_speed;
			}
			else// don't move if close enough
			{
				// XBM: look around
				pBot->f_move_speed = 0.0f;
				/*if(pBot->f_follow_look_time <= gpGlobals->time)
				{
					pBot->f_follow_look_time = gpGlobals->time + RANDOM_FLOAT(2.0, 5.0);
					pBot->b_follow_look = true;
				}*/
			}

			// duck along with player if nearby
			if (g_bot_follow_actions.value > 0.0f)
			{
				if (FBitSet(pBot->pBotUser->v.flags, FL_DUCKING))
					pEdict->v.button |= IN_DUCK;
				else if (!IsOnLadder(pEdict) && !FBitSet(pBot->pBotUser->v.flags, FL_ONGROUND) && pBot->pBotUser->v.velocity.z >= 0.0f)// XDM3038
					pEdict->v.button |= IN_JUMP;
				//no need to	else if (pEdict->v.button & IN_DUCK)
					//pEdict->v.button &= ~IN_DUCK;

				if (((pBot->pBotUser->v.effects & EF_DIMLIGHT) != 0) != ((pEdict->v.effects & EF_DIMLIGHT) != 0))
						pEdict->v.impulse = 100;// XDM3037: this emulates impulse command

				/* "direct" version, no flashlight toggling, just effect. May cause some trouble.
				if (pBot->pBotUser->v.effects & EF_DIMLIGHT)
					pEdict->v.effects |= EF_DIMLIGHT;
				else if (pEdict->v.effects & EF_DIMLIGHT)
					pEdict->v.effects &= ~EF_DIMLIGHT;*/
			}
		}

		if (IsOnLadder(pEdict))// XDM3037a
		{
			// player's origin is at feet
			bool bClose = ((pEdict->v.origin.z - pBot->pBotUser->v.origin.z) < (PLAYER_HEIGHT*2));

			if (bClose)// try mimic player's movements
			{
				if (pBot->pBotUser->v.button & IN_JUMP)
					pEdict->v.button |= IN_JUMP;
				else if (pBot->pBotUser->v.velocity.z < 0)
					pBot->ladder_dir = LADDER_DOWN;
				else if (pBot->pBotUser->v.velocity.z > 0)
					pBot->ladder_dir = LADDER_UP;
			}
			else// try to catch up with player
			{
				if ((pEdict->v.origin.z - pBot->pBotUser->v.origin.z) > PLAYER_HEIGHT*2)// bot is above
					pBot->ladder_dir = LADDER_DOWN;
				else if ((pBot->pBotUser->v.origin.z - pEdict->v.origin.z) > PLAYER_HEIGHT*2)// bot is below
					pBot->ladder_dir = LADDER_UP;
			// check if it's safe to jump!!	else if (user_visible && (f_distance > g_bot_follow_distance.value*0.75f))//IsFacing(pEdict->v.origin + pEdict->v.view_ofs, pEdict->v.v_angle, vecEnd))
			//		pEdict->v.button |= IN_JUMP;// try jumping off the ladder
				else
					pBot->ladder_dir = LADDER_UNKNOWN;
			}
		}
		else if (pEdict->v.waterlevel >= 3)// XDM3038
		{
			if (g_psv_maxspeed)
			{
				if (pEdict->v.origin.z > (pBot->pBotUser->v.origin.z + PLAYER_HEIGHT + HULL_RADIUS))
					pBot->cmd_upmove = -g_psv_maxspeed->value;
				else if (pEdict->v.origin.z < (pBot->pBotUser->v.origin.z - HULL_RADIUS))// player' origin is at feet
					pBot->cmd_upmove = g_psv_maxspeed->value;
				else
					pBot->cmd_upmove = 0;
			}
			/*pBot->pEdict->v.idealpitch = -90;
			// swim up towards the surface
			pBot->pEdict->v.v_angle.x = -80;// look upwards
			// move forward (i.e. in the direction the bot is looking, up or down)
			pBot->pEdict->v.button |= (IN_FORWARD | IN_JUMP);*/
		}
		return true;
	}
	else// person to follow has gone out of sight...
	{
//		conprintf(2, "XBM: bot %s lost user (%s)\n", STRING(pBot->pEdict->v.netname), STRING(pBot->pBotUser->v.netname));
//		FakeClientCommand(pBot->pEdict, "say_private", itoa(ENTINDEXpBot->pBotUser)), "Hey! Where did you go?!");
//		BotSpeakDirect(pBot, "Hey! Where did you go?!", 0.0);
// XBM pBot->pBotUser = NULL;
		return false;
	}
}

void BotHandleWalls(bot_t *pBot, vec_t &moved_distance)
{
	DBG_BOT_PRINT("BotHandleWalls(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	TraceResult tr;
	// check if we should be avoiding walls
	if (pBot->f_dont_avoid_wall_time <= gpGlobals->time)
	{
		// let's just randomly wander around
		if (BotStuckInCorner(pBot))
		{
			pEdict->v.ideal_yaw += 180.0f;  // turn 180 degrees
			BotFixIdealYaw(pEdict);
			pBot->f_move_speed = 0;  // don't move while turning
			pBot->f_dont_avoid_wall_time = gpGlobals->time + 1.0f;
			moved_distance = 2.0f;  // dont use bot stuck code
		}
		else
		{
			// check if there is a wall on the left...
			if (!BotCheckWallOnLeft(pBot))
			{
				// if there was a wall on the left over 1/2 a second ago then
				// 20% of the time randomly turn between 45 and 60 degrees
				if ((pBot->f_wall_on_left != 0) && (pBot->f_wall_on_left <= gpGlobals->time - 0.5) && (RANDOM_LONG(1, 100) <= 20))
				{
					pEdict->v.ideal_yaw += RANDOM_LONG(45, 60);
					BotFixIdealYaw(pEdict);
					pBot->f_move_speed = 0;  // don't move while turning
					pBot->f_dont_avoid_wall_time = gpGlobals->time + 1.0f;
				}
				pBot->f_wall_on_left = 0;  // reset wall detect time
			}
			else if (!BotCheckWallOnRight(pBot))
			{
				// if there was a wall on the right over 1/2 a second ago then
				// 20% of the time randomly turn between 45 and 60 degrees
				if ((pBot->f_wall_on_right != 0) && (pBot->f_wall_on_right <= gpGlobals->time - 0.5f) && (RANDOM_LONG(1, 100) <= 20))
				{
					pEdict->v.ideal_yaw -= RANDOM_LONG(45, 60);
					BotFixIdealYaw(pEdict);
					pBot->f_move_speed = 0;  // don't move while turning
					pBot->f_dont_avoid_wall_time = gpGlobals->time + 1.0f;
				}
				pBot->f_wall_on_right = 0.0f;  // reset wall detect time
			}
		}
	}
	// check if bot is about to hit a wall.  TraceResult gets returned
	if ((pBot->f_dont_avoid_wall_time <= gpGlobals->time) && BotCantMoveForward(pBot, &tr))
	{
		// XDM3035c
		bool bCrouchJump;
		if (BotCanJumpUp(pBot, &bCrouchJump))
		{
			if ((pBot->f_jump_time + 2.0f) <= gpGlobals->time)
			{
				pBot->f_jump_time = gpGlobals->time;
				pEdict->v.button |= IN_JUMP;  // jump up and move forward

				if (bCrouchJump)
					pEdict->v.button |= IN_DUCK;  // also need to duck
			}
		}
		else if (BotCanDuckUnder(pBot))
		{
			pEdict->v.button |= IN_DUCK;
		}
		else
			BotTurnAtWall(pBot, &tr, true);
	}
}

bool BotCheckWallOnLeft(bot_t *pBot)
{
	DBG_BOT_PRINT("BotCheckWallOnLeft(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	Vector v_src(pEdict->v.origin);
	Vector v_left;
	TraceResult tr;
	UTIL_MakeVectors( pEdict->v.v_angle );
	// do a trace to the left...
	v_left = v_src + gpGlobals->v_right * -40;  // 40 units to the left
	UTIL_TraceLine( v_src, v_left, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
	// check if the trace hit something...
	if (tr.flFraction < 1.0)
	{
		if (pBot->f_wall_on_left < 1.0)
			pBot->f_wall_on_left = gpGlobals->time;

		DBG_BOT_PRINT("BotCheckWallOnLeft(%s): found\n", pBot->name);
		return true;
	}
	return false;
}

bool BotCheckWallOnRight(bot_t *pBot)
{
	DBG_BOT_PRINT("BotCheckWallOnRight(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	Vector v_src, v_right;
	TraceResult tr;
	UTIL_MakeVectors( pEdict->v.v_angle );
	// do a trace to the right...
	v_src = pEdict->v.origin;
	v_right = v_src + gpGlobals->v_right * 40;  // 40 units to the right
	UTIL_TraceLine( v_src, v_right, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
	// check if the trace hit something...
	if (tr.flFraction < 1.0)
	{
		if (pBot->f_wall_on_right < 1.0f)
			pBot->f_wall_on_right = gpGlobals->time;

		DBG_BOT_PRINT("BotCheckWallOnRight(%s): found\n", pBot->name);
		return true;
	}
	return false;
}

// mind the gap, check for ledge
bool BotLookForDrop(bot_t *pBot)
{
	DBG_BOT_PRINT("BotLookForDrop(%s)\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	Vector v_src, v_dest, v_ahead;
	float scale, direction;
	TraceResult tr;
	int contents;
	int turn_count;
	bool need_to_turn = false, done = false;

	scale = 80 + pBot->f_max_speed*0.1f;
	v_ahead = pEdict->v.v_angle;
	v_ahead.x = 0;// set pitch to level horizontally
	UTIL_MakeVectors(v_ahead);
	v_src = pEdict->v.origin;
	v_dest = v_src + gpGlobals->v_forward * scale;
	UTIL_TraceLine(v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr);

	// check if area in front of bot was clear...
	if (tr.flFraction >= 1.0) 
	{
		v_src = v_dest;  // start downward trace from endpoint of open trace
		v_dest.z = v_dest.z - max_drop_height;
		UTIL_TraceLine( v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr );
		//need_to_turn = false;
		// if trace did not hit anything then drop is TOO FAR...
		if (tr.flFraction >= 1.0) 
			need_to_turn = true;
		else
		{
			// we've hit something, see if it's water or lava
			contents = POINT_CONTENTS( tr.vecEndPos );
			if (contents == CONTENTS_SLIME)
				need_to_turn = true;
			else if (contents == CONTENTS_LAVA)
				need_to_turn = true;
			//else if (contents == CONTENTS_WATER)// if you don't like water, set need_to_turn = true here
		}

		if (need_to_turn)
		{
			pBot->f_move_speed *= 0.25f;// XDM3035a: slow down!

			// if we have an enemy, stop heading towards enemy...
			if (pBot->pBotEnemy)
			{
				pBot->pBotEnemy = NULL;
				pBot->f_bot_find_enemy_time = gpGlobals->time + 1.0f;
			}
			// don't look for items for a while...
			pBot->f_find_item = gpGlobals->time + 1.0f;
			// change the bot's ideal yaw by finding surface normal
			// slightly below where the bot is standing
			v_dest = pEdict->v.origin;

			if (pEdict->v.flags & FL_DUCKING)
			{
				v_src.z -= 22;  // (36/2) + 4 units
				v_dest.z -= 22;
			}
			else
			{
				v_src.z -= 40;  // (72/2) + 4 units
				v_dest.z -= 40;
			}

			UTIL_TraceLine( v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr );

			if (tr.flFraction < 1.0)// hit something the bot is standing on...
				BotTurnAtWall(pBot, &tr, false);
			else
			{
				// pick a random direction to turn...
				if (RANDOM_LONG(1, 100) <= 50)
					direction = 1.0f;
				else
					direction = -1.0f;

				// turn 30 degrees at a time until bot is on solid ground
				v_ahead = pEdict->v.v_angle;
				v_ahead.x = 0;  // set pitch to level horizontally
				done = false;
				turn_count = 0;
				
				while (!done)
				{
					v_ahead.y += 30.0f * direction;
					NormalizeAngle360(&v_ahead.y);
					/*if (v_ahead.y > 360.0f)
						v_ahead.y -= 360.0f;
					else if (v_ahead.y < -360.0f) WTF?!
						v_ahead.y += 360.0f;*/

					UTIL_MakeVectors(v_ahead);
					v_src = pEdict->v.origin;
					v_dest = v_src + gpGlobals->v_forward * scale;
					UTIL_TraceLine( v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr );
					// check if area in front of bot was clear...
					if (tr.flFraction >= 1.0) 
					{
						v_src = v_dest;  // start downward trace from endpoint of open trace
						v_dest.z = v_dest.z - max_drop_height;
						UTIL_TraceLine( v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr );
						// if trace hit something then drop is NOT TOO FAR...
						if (tr.flFraction >= 1.0) 
							done = true;
					}
					turn_count++;
					if (turn_count == 6)  // 180 degrees? (30 * 6 = 180)
						done = true;
				}
				pBot->pEdict->v.ideal_yaw = v_ahead.y;
				BotFixIdealYaw(pEdict);
			}
		}
	}
	return need_to_turn;
}

bool BotHoldPosition(bot_t *pBot)// XBM
{
//	DBG_BOT_PRINT("BotHoldPosition(%s): found\n", pBot->name);
	edict_t *pEdict = pBot->pEdict;
	if (FVisible(pBot->posHold.origin, pEdict) || (pBot->f_bot_use_time + 5.0f > gpGlobals->time))
	{
		pBot->f_bot_use_time = gpGlobals->time;// reset "last visible time"
		Vector v_pos = pBot->posHold.origin - pEdict->v.origin;
		//Vector bot_angles = UTIL_VecToAngles(v_pos);
		//pEdict->v.ideal_yaw = bot_angles.y;
		pEdict->v.ideal_yaw = VecToYaw(v_pos);
		BotFixIdealYaw(pEdict);
		vec_t f_distance = v_pos.Length();// how far away

		if (f_distance > 200)
			pBot->f_move_speed = pBot->f_max_speed;
		else if (f_distance > 56)
			pBot->f_move_speed = pBot->f_max_speed / 2;
		else
		{
			pBot->f_move_speed = 0.0;
			pBot->pEdict->v.idealpitch = pBot->posHold.angles.x;
			pBot->pEdict->v.ideal_yaw = pBot->posHold.angles.y;
			if (pBot->posHold.flags & FL_DUCKING)
				pBot->f_duck_time = gpGlobals->time + 1.0f;// continue ducking
		}
		return true;
	}
	return false;
}
