/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************
Adapted from:
   HPB_bot
   botman's High Ping Bastard bot
   http://planethalflife.com/botman/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/bot_navigate.cpp,v $
$Revision: 1.9 $
$Date: 2001/05/11 06:17:23 $
$State: Exp $

***********************************

$Log: bot_navigate.cpp,v $
Revision 1.9  2001/05/11 06:17:23  eric
navigation cleanup

Revision 1.8  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.7  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.6  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

Revision 1.5  2001/04/30 08:06:11  eric
willFire() deals with crouching versus standing

Revision 1.4  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.3  2001/04/29 01:45:36  eric
Tweaked CVS info headers

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "ai.h"
#include "ai_behavior.h"
#include "ai_squad_func.h"
#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"

extern bot_t bots[MAX_BOTS];
extern squad_t squads[MAX_SQUADS];
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern num_waypoints;
extern bool bomb_planted;


/**
 * Indicates that the given squad
 * needs fresh navigational objectives.
 */
void resetSquadNav (squad_t *pSquad) {
	if (findRandomSquadMember (pSquad) != NULL) {
		reacessSquadGoal (pSquad);
		pSquad->wp_intermediate = WaypointIntermediate (pSquad);
		pSquad->wp_destination = WaypointByObjective (pSquad);
	}
}

/**
 * Called to indicate that all squads
 * should reconsider their navigational
 * objectives.
 */
void resetAllSquadsNav (void) {
	for (int i = 0; i < MAX_SQUADS; i++) {
		if (bots[i].is_used) // only is_used indices are guaranteed to exist
			resetSquadNav (&squads[i]);
	}
}

/**
 * Returns the current destination for
 * the given squad, choosing between the
 * intermediate and final destinations.
 */
int currentDestination (squad_t *pSquad) {
	if (validWaypoint (pSquad->wp_intermediate))
		return pSquad->wp_intermediate;
	else
		return pSquad->wp_destination;
}

/**
 * Called to inform the squad that it's
 * foremost member has reached one waypoint
 * closer to it's squad's destination.
 */
void squadCloser (squad_t *pSquad, int wp) {
	if (pSquad == NULL || !validWaypoint (wp))
		return;

	if (wp == pSquad->wp_intermediate)
		pSquad->wp_intermediate = WP_INVALID;

	if (wp == pSquad->wp_destination) {
		if (pSquad->goal == X_DEFEND) {
			bot_t *pBot = findRandomSquadMember (pSquad);

			if (pBot != NULL) {
				if (randomize (1))
					radioOverride (pBot, RANDOM_FLOAT (0.5, 1.0), "1", "3"); // get in position
				else
					radioOverride (pBot, RANDOM_FLOAT (0.5, 1.0), "2", "4"); // hold this position
			}

			ambush (pBot);
		} else {
			pSquad->wp_destination = WP_INVALID;
		}
	}

	if (! validWaypoint (pSquad->wp_destination))
		resetSquadNav (pSquad);

	if (wp == pSquad->wp_current || !validWaypoint (pSquad->wp_current))
		pSquad->wp_current = WaypointRouteFromTo (wp, currentDestination (pSquad));
}

/** Generates a random waypoint offset. */
Vector waypointOffset (void) {
	float d = 5.0;
	return Vector (RANDOM_FLOAT (-d, d), RANDOM_FLOAT (-d, d), 0);
}

/**
 * Returns true iff another bot has
 * already claimed residency of this waypoint.
 */
bool wpClaimed (bot_t *pBot, int wp) {
	for (int i = 0; i < MAX_BOTS; i++) {
		bot_t *bot = &bots[i];
		if (bot->is_used
			&& IsAlive (bot->pEdict)
			&& pBot != bot)
		{
			if (bot->wp_current == wp)
				return true;
		}
	}

	return false;
}

/**
 * Returns an alternate next waypoint
 * if one exists. Returns WP_INVALID to indicate
 * no alternate exists.
 */
int WaypointAlternate (bot_t *pBot, int wp_from, int wp_next) {
	// TODO: ideally this should be random (not pick the lowest index)

	if (validWaypoint (wp_next)) {
		for (int i = 0; i < num_waypoints; i++) {
			if (i == wp_from
				|| i == wp_next
				|| wpClaimed (pBot, i)
				|| !isPath (wp_from,i))
			{
				continue;
			}

			if (isPath (i, wp_next))
				return i;

			int wp_next2 = WaypointRouteFromTo (wp_next, pBot->pSquad->wp_current);
			if (validWaypoint (wp_next2) && isPath (i, wp_next2))
				return i;

			for (int j = 0; j < num_waypoints; j++) {
				if (j == wp_from
					|| j == wp_next
					|| j == i
					|| !isPath (wp_from, i)
					|| !isPath (i, j))
				{
					continue;
				}

				if (isPath (j, wp_next))
					return i;
				
				if (validWaypoint (wp_next2)) {
					if (isPath (j, wp_next2))
						return i;

					int wp_next3 = WaypointRouteFromTo (wp_next2, pBot->pSquad->wp_current);
					if (validWaypoint (wp_next3) && isPath (j, wp_next3))
						return i;
				}
			}
		}
	}

	return wp_next; // no alternate
}

/** Called to set the current (or next) waypoint. */
void setCurrentWaypoint (bot_t *pBot, int wp) {
	pBot->wp_last = pBot->wp_current;
/*
	if (wpClaimed (pBot, wp))
		wp = WaypointAlternate (pBot, pBot->wp_last, wp);
*/
	pBot->wp_current = wp;
	if (validWaypoint (pBot->wp_current))
		pBot->wp_origin = waypoints[pBot->wp_current].origin + waypointOffset ();
	else
		pBot->wp_origin = Vector (0, 0, 0);

	// is the squad as a whole now closer?
	squad_t *pSquad = pBot->pSquad;
	if (pSquad != NULL && (pBot->wp_last == pSquad->wp_current || !validWaypoint (pSquad->wp_current))) {
		squadCloser (pSquad, pBot->wp_last);
	}
}

/**
 * Returns the next waypoint the bot
 * should head towards.
 */
int BotWaypointNext (bot_t *pBot) {
	int wp_immediate;
	if (validWaypoint (pBot->wp_tactical) && pBot->tactical_time > gpGlobals->time)
		wp_immediate = pBot->wp_tactical;
	else if (pBot->pSquad != NULL && validWaypoint (pBot->pSquad->wp_current))
		wp_immediate = pBot->pSquad->wp_current;
	else
		return WP_INVALID;

	return WaypointRouteFromTo (pBot->wp_current, wp_immediate);
}

/**
 * Checks to see if the bot is already closer
 * to her current waypoint. If so, the current
 * waypoint is updated.
 */
void alreadyCloser (bot_t *pBot) {
	int next = BotWaypointNext (pBot);
	if (validWaypoint (next) && visibleWaypoint (pBot, next)) {
		float d_from_me = distance (pBot->pEdict->v.origin, waypoints[next].origin);
		float d_from_wp = distance (pBot->wp_origin, waypoints[next].origin);
		if (d_from_me <= d_from_wp)
			setCurrentWaypoint (pBot, next);
	}
}

/**
 * Keeps both the current and destination
 * waypoints up to date. Returns true iff
 * the bot currently knows where it should
 * be going.
 */
bool BotHeadTowardWaypoint (bot_t *pBot) {
	edict_t *pEdict = pBot->pEdict;
	squad_t *pSquad = pBot->pSquad;

	// need wp_current
	int nearest_wp = WaypointFindNearest (pEdict, REACHABLE_RANGE);
	if (!validWaypoint (pBot->wp_current)
		|| !visibleWaypoint (pBot, pBot->wp_current)
		|| (nearest_wp != pBot->wp_current && nearest_wp != pBot->wp_last))
	{
		if (validWaypoint (nearest_wp)) {
			if (pBot->wp_current == nearest_wp) {
				setCurrentWaypoint (pBot, WP_INVALID);
				return false;
			}

			setCurrentWaypoint (pBot, nearest_wp);
		}
	}

	if (validWaypoint (pBot->wp_current)) {
		alreadyCloser (pBot);

		// at wp_current
		float distance = (pEdict->v.origin - pBot->wp_origin).Length();
		if (distance < WP_TOUCH_DISTANCE) {
			setCurrentWaypoint (pBot, BotWaypointNext (pBot));
		}
	}

	return validWaypoint (pBot->wp_current);
}

/** Returns true iff the trace hits something. */
bool traceHit (Vector src, Vector dest, edict_t *containing, bool ignore) {
	TraceResult tr;

	IGNORE_MONSTERS im;
	if (ignore)
		im = ignore_monsters;
	else
		im = dont_ignore_monsters;

	UTIL_TraceLine (src, dest, im, containing, &tr);

	if (tr.flFraction < 1.0)
		return true;
	else
		return false;
}

/** Returns true iff the trace hits something. */
bool traceHit (Vector src, Vector dest, edict_t *containing) {
	return traceHit (src, dest, containing, false);
}

/** Requires that UTIL_MakeVectors has already been called. */
bool areaClear (Vector origin, edict_t *container, float height, float side, float forward) {
	// center
	Vector v_src = origin + Vector (0, 0, height);
	Vector v_dest = v_src + gpGlobals->v_forward * forward;
	if (traceHit (v_src, v_dest, container))
		return false;

	// right
	v_src = origin + gpGlobals->v_right * side + Vector (0, 0, height);
	v_dest = v_src + gpGlobals->v_forward * forward;
	if (traceHit (v_src, v_dest, container))
		return false;

	// left
	v_src = origin + gpGlobals->v_right * -side + Vector (0, 0, height);
	v_dest = v_src + gpGlobals->v_forward * forward;
	if (traceHit (v_src, v_dest, container))
		return false;

	return true;
}

/**
 * What I do here is trace 3 lines straight out, one unit higher than
 * the highest normal jumping distance.  I trace once at the center of
 * the body, once at the right side, and once at the left side.  If all
 * three of these TraceLines don't hit an obstruction then I know the
 * area to jump to is clear.  I then need to trace from head level,
 * above where the bot will jump to, downward to see if there is anything
 * blocking the jump.  There could be a narrow opening that the body
 * will not fit into.  These horizontal and vertical TraceLines seem
 * to catch most of the problems with falsely trying to jump on something
 * that the bot can not get onto. [botman]
 */
bool obstacleJump (bot_t *pBot) {
	static const float MID_HEIGHT = 36.0;
	static const float JUMP_HEIGHT = 45.0;
	static const float MAX_JUMP_HEIGHT = (JUMP_HEIGHT + 1) - MID_HEIGHT;
	static const float JUMP_LIMIT_HEIGHT = JUMP_HEIGHT - MID_HEIGHT;
	static const float HEAD_OFFSET = 36.0;
	static const float ABOVE_OFFSET = 72.0;
	static const float ABOVE_HEAD_OFFSET = HEAD_OFFSET + ABOVE_OFFSET;
	static const float SIDE_DISTANCE = 16.0;
	static const float FORWARD_DISTANCE = 24.0;

	Vector v_src, v_dest;
	edict_t *pEdict = pBot->pEdict;
	Vector origin = pEdict->v.origin;
	edict_t *container = pEdict->v.pContainingEntity;

	Vector v_jump = pEdict->v.v_angle;
	v_jump.x = 0; // reset pitch to 0 (level horizontally)
	v_jump.z = 0; // reset roll to 0 (straight up and down)
	UTIL_MakeVectors (v_jump);

	if (! areaClear (origin, container, MAX_JUMP_HEIGHT, SIDE_DISTANCE, FORWARD_DISTANCE))
		return false;

	// head level downward
	v_src = origin + gpGlobals->v_forward * FORWARD_DISTANCE;
	v_src.z = v_src.z + ABOVE_HEAD_OFFSET;
	v_dest = v_src + Vector (0, 0, -(ABOVE_HEAD_OFFSET - JUMP_LIMIT_HEIGHT));
	if (traceHit (v_src, v_dest, container))
		return false;

	// right downward
	v_src = origin + gpGlobals->v_right * SIDE_DISTANCE + gpGlobals->v_forward * FORWARD_DISTANCE;
	v_src.z = v_src.z + ABOVE_HEAD_OFFSET;
	v_dest = v_src + Vector (0, 0, -(ABOVE_HEAD_OFFSET - JUMP_LIMIT_HEIGHT));
	if (traceHit (v_src, v_dest, container))
		return false;

	// left downward
	v_src = origin + gpGlobals->v_right * -SIDE_DISTANCE + gpGlobals->v_forward * FORWARD_DISTANCE;
	v_src.z = v_src.z + ABOVE_HEAD_OFFSET;
	v_dest = v_src + Vector (0, 0, -(ABOVE_HEAD_OFFSET - JUMP_LIMIT_HEIGHT));
	if (traceHit (v_src, v_dest, container))
		return false;

	return true;
}

/**
 * What I do here is trace 3 lines straight out, one unit higher than
 * the ducking height.  I trace once at the center of the body, once
 * at the right side, and once at the left side.  If all three of these
 * TraceLines don't hit an obstruction then I know the area to duck to
 * is clear.  I then need to trace from the ground up, 72 units, to make
 * sure that there is something blocking the TraceLine.  Then we know
 * we can duck under it. [botman]
 */
bool obstacleDuck (bot_t *pBot) {
	static const float MID_OFFSET = -36.0;
	static const float DUCK_HEIGHT = 36.0;
	static const float DUCK_LIMIT_HEIGHT = (DUCK_HEIGHT + 1) + MID_OFFSET;
	static const float SIDE_DISTANCE = 16.0;
	static const float FORWARD_DISTANCE = 24.0;

	Vector v_src, v_dest;
	edict_t *pEdict = pBot->pEdict;
	Vector origin = pEdict->v.origin;
	edict_t *container = pEdict->v.pContainingEntity;

	Vector v_duck = pEdict->v.v_angle;
	v_duck.x = 0; // reset pitch to 0 (level horizontally)
	v_duck.z = 0; // reset roll to 0 (straight up and down)
	UTIL_MakeVectors (v_duck);

	if (! areaClear (origin, container, DUCK_LIMIT_HEIGHT, SIDE_DISTANCE, FORWARD_DISTANCE))
		return false;

	// trace from the ground up to check for an obstacle to duck underneath

	// start of trace is FORWARD_DISTANCE units in front of bot near ground...
	v_src = origin + gpGlobals->v_forward * FORWARD_DISTANCE;
	v_src.z = v_src.z + (MID_OFFSET + 1);
	v_dest = v_src + Vector (0, 0, PLAYER_HEIGHT);
	if (! traceHit (v_src, v_dest, container))
		return false;

	// right
	v_src = origin + gpGlobals->v_right * SIDE_DISTANCE + gpGlobals->v_forward * FORWARD_DISTANCE;
	v_src.z = v_src.z + (MID_OFFSET + 1);
	v_dest = v_src + Vector (0, 0, PLAYER_HEIGHT);
	if (! traceHit (v_src, v_dest, container))
		return false;

	// left
	v_src = origin + gpGlobals->v_right * -SIDE_DISTANCE + gpGlobals->v_forward * FORWARD_DISTANCE;
	v_src.z = v_src.z + (MID_OFFSET + 1);
	v_dest = v_src + Vector (0, 0, PLAYER_HEIGHT);
	if (! traceHit (v_src, v_dest, container))
		return false;

	return true;
}

bool obstacleFront (bot_t *pBot) {
	edict_t *pEdict = pBot->pEdict;

	UTIL_MakeVectors (pEdict->v.v_angle);
	Vector forward = gpGlobals->v_forward * 30;

	return traceHit (pEdict->v.origin, pEdict->v.origin + forward, pEdict->v.pContainingEntity);
}

bool obstacleSide (bot_t *pBot, bool left) {
	edict_t *pEdict = pBot->pEdict;

	UTIL_MakeVectors (pEdict->v.v_angle);
	Vector forward = gpGlobals->v_forward * 20;
	Vector side = gpGlobals->v_right * 20;
	if (left)
		side = side * -1.0;

	return traceHit (pEdict->v.origin, pEdict->v.origin + forward + side, pEdict->v.pContainingEntity);
}

bool obstacleLeft (bot_t *pBot) {
	return obstacleSide (pBot, true);
}

bool obstacleRight (bot_t *pBot) {
	return obstacleSide (pBot, false);
}
