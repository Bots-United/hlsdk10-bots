/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_move.cpp,v $
$Revision: 1.11 $
$Date: 2001/05/10 10:00:25 $
$State: Exp $

***********************************

$Log: ai_move.cpp,v $
Revision 1.11  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.10  2001/05/03 19:35:16  eric
CGF-style aiming

Revision 1.9  2001/05/02 13:09:31  eric
Housekeeping

Revision 1.8  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.7  2001/04/30 08:06:10  eric
willFire() deals with crouching versus standing

Revision 1.6  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.5  2001/04/29 04:13:41  eric
Trimmed down bot structure

Revision 1.4  2001/04/29 03:01:46  eric
Fixed lookAhead() to consider squad WPs correctly

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
#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"

extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;

/**
 * Returns the equivalent angle bound
 * to +-180 degrees. (courtesy Paulo-La-Frite)
 */
Vector clampAngleTo180 (Vector angle) {
	// clamp X
	while (angle.x > 180) {
		angle.x -=360;
		// (-oo,180]
	}

	while (angle.x <= -180) {
		angle.x +=360;
		// (-180,180]
	}

	// clamp Y
	while (angle.y > 180) {
		angle.y -=360;
		// (-oo,180]
	}

	while (angle.y <= -180) {
		angle.y +=360;
		// (-180,180]
	}

	// clamp Z
	while (angle.z > 180) {
		angle.z -=360;
		// (-oo,180]
	}

	while (angle.z <= -180) {
		angle.z +=360;
		// (-180,180]
	}

	return angle;
}

/**
 * Produces a smoother version of its input values.
 *
 * @param c  The constant is bound to [0, 1] and
 *     determines how quickly the smoothing responds
 *     to newer values. A constant of zero returns
 *     "x", while a constant of one returns "old".
 */
Vector mavg (Vector old, Vector x, float c) {
	if (c > 1.0)
		c = 1.0;
	else if (c < 0.0)
		c = 0.0;

	return ((c * old) + ((1 - c) * x));
}

/**
 * Produces a smoother version of its input values.
 *
 * @param all  Array values are expected to be in
 *     chronological order (i.e. the newest item
 *     is at the end of the array).
 */
Vector mavg (Vector all[], int size, float c) {
	if (size < 1)
		return Vector (0, 0, 0); // TODO: throw exception
	if (size < 2)
		return all[0];

	Vector temp = all[0];
	for (int i = 1; i < size; i++)
		temp = mavg (temp, all[i], c);
	return temp;
}
	
/**
 * Smooths the given looking vector by
 * taking into account the past.
 */
Vector smoothLook (bot_t *pBot) {
	float c = 0.5; // [0,1] -- zero = instant response

	Vector coarse[VIEW_HISTORY_SIZE + 1];

	for (int i = 0; i < VIEW_HISTORY_SIZE; i++) {
		UTIL_MakeVectors (pBot->prev_v_angle[i]);
		coarse[i] = gpGlobals->v_forward; // X inverted
	}

	Vector new_angle = UTIL_VecToAngles (pBot->lookAt);
	new_angle = clampAngleTo180 (new_angle); // X prime
	new_angle.x = -new_angle.x; // X inverted

	UTIL_MakeVectors (new_angle);
	coarse[VIEW_HISTORY_SIZE] = gpGlobals->v_forward;

	Vector smooth = mavg (coarse, VIEW_HISTORY_SIZE + 1, c);

	return UTIL_VecToAngles (smooth);
}

/**
 * Smooths the given movement vector
 * by taking into account the past.
 */
Vector smoothVelocity (bot_t *pBot) {
	float c = 0.5; // [0,1] -- zero = instant response

	float speed = (pBot->desiredVelocity).Length ();

	Vector coarse[VEL_HISTORY_SIZE + 1];

	for (int i = 0; i < VEL_HISTORY_SIZE; i++) {
		Vector p = pBot->prev_velocity[i];
		p.z = 0; // horizontal plane only
		coarse[i] = (p).Normalize ();
		coarse[i] = coarse[i] * speed;
	}

	coarse[VEL_HISTORY_SIZE] = pBot->desiredVelocity;
	coarse[VEL_HISTORY_SIZE].z = 0; // horizontal plane only
	coarse[VEL_HISTORY_SIZE] = (coarse[VEL_HISTORY_SIZE]).Normalize ();
	coarse[VEL_HISTORY_SIZE] * speed;

	Vector smooth = mavg (coarse, VEL_HISTORY_SIZE + 1, c);

	return UTIL_VecToAngles (smooth);
}

/** Is the waypoint visible to this bot? */
bool visibleWaypoint (bot_t *pBot, int waypoint) {
	return FVisible (waypoints[waypoint].origin, pBot->pEdict);
}


/** Sanity check THIS waypoint. TRUE indicates sanity. */
bool validWaypoint (int waypoint)
{
	if ((waypoint > WP_INVALID) && (waypoint < num_waypoints))
		return true;
	else
		return false;
}

/**
 * Returns the waypoint which is the
 * furthest along the route from start
 * to the end which is still visible.
 */
int farthestVisible (bot_t *viewer, int start, int end) {
	if (!validWaypoint (start) || !validWaypoint (end))
		return WP_INVALID;

	int furthest = start;
	for (int i = 0; i < 50; i++) { // look no more than 50 wp's ahead
		int possibly_further = WaypointRouteFromTo (furthest, end);

		if (validWaypoint (possibly_further) && visibleWaypoint (viewer, possibly_further))
			furthest = possibly_further;
		else
			break;
	}

	return furthest;
}

/** Returns the location of the entity's eyes. */
Vector eyePosition (edict_t *pEdict) {
	return (pEdict->v.origin + pEdict->v.view_ofs);
}

/**
 * Returns a looking vector aimed at the
 * furthest waypoint along the bot's path
 * that it can see.
 */
Vector lookAhead (bot_t *pBot) {
	squad_t *pSquad = pBot->pSquad;

	if (pSquad == NULL)
		return pBot->lookAt;

	int wp_farthest = farthestVisible (pBot, pBot->wp_current, pSquad->wp_current);
	if (wp_farthest == pSquad->wp_current)
		wp_farthest = farthestVisible (pBot, pSquad->wp_current, currentDestination (pSquad));

	if (! validWaypoint (wp_farthest))
		return pBot->lookAt;

	return waypoints[wp_farthest].origin - eyePosition (pBot->pEdict);
}

/**
 * Returns a looking vector aimed at the
 * current threat.
 */
Vector lookThreat (bot_t *pBot) {
	if (pBot->threat == NULL)
		return pBot->lookAt;

	Vector look_location;

	if (inSight (pBot, pBot->threat)) {
		look_location = pBot->threat->v.origin;
	} else if (FVisible (pBot->last_seen_threat_location, pBot->pEdict)) {
		look_location = pBot->last_seen_threat_location;
	} else {
		int wp_threat = WaypointFindNearest (pBot->threat, REACHABLE_RANGE);
		int wp_prediction = farthestVisible (pBot, pBot->wp_current, wp_threat);
		if (validWaypoint (wp_prediction))
			look_location = waypoints[wp_prediction].origin;
		else
			return pBot->lookAt;
	}
/*
	// aim less for the head as distance increases
	static float MAX_HEADSHOT_DISTANCE = (MAX_DISTANCE / 2.0);
	float d = distance (pBot->threat->v.origin, pBot->pEdict->v.origin);
	d = clamp (d, 0.0, MAX_HEADSHOT_DISTANCE);
	float scale = (1.0 - (d / MAX_HEADSHOT_DISTANCE)); // [0,1]
*/
	// TODO: tease aim downward when using guns with kick

	look_location = look_location
//		+ (pBot->aim_head * scale)
		+ (pBot->aim_deviation * aimDeviationScale (pBot, inSight (pBot, pBot->threat)));

	return look_location - eyePosition (pBot->pEdict);
}

/** Returns the origin of the given entity. */
Vector getOrigin (edict_t *pEdict) {
	if (pEdict == NULL)
		return Vector (0, 0, 0); // TODO: should throw exception

	if (isWeapon (pEdict))
		return pEdict->v.owner->v.origin;
	else if (isFunc (pEdict))
		return VecBModelOrigin (pEdict);
	else
		return pEdict->v.origin;
}

/** Returns the origin of the bot's pickup. */
Vector pickupOrigin (bot_t *pBot) {
	if (pBot->pickup == NULL)
		return Vector (0, 0, 0); // TODO: should throw exception

	return getOrigin (pBot->pickup);
}

/** Returns the looking vector aimed at the current pickup. */
Vector lookPickup (bot_t *pBot) {
	if (pBot->pickup == NULL) {
		return pBot->lookAt;
	}

	edict_t *pEdict = pBot->pEdict;

	return pickupOrigin (pBot) - eyePosition (pEdict);
}
