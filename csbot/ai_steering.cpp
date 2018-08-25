/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************
Taken from Craig W. Reynolds's paper
entitled "Steering Behaviors for
Autonomous Characters".
	cwr@red.com
	www.red.com/cwr
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_steering.cpp,v $
$Revision: 1.9 $
$Date: 2001/05/11 06:17:23 $
$State: Exp $

***********************************

$Log: ai_steering.cpp,v $
Revision 1.9  2001/05/11 06:17:23  eric
navigation cleanup

Revision 1.8  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.7  2001/05/02 13:09:31  eric
Housekeeping

Revision 1.6  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.5  2001/04/30 08:06:10  eric
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

extern WAYPOINT waypoints[MAX_WAYPOINTS];

/** Toward a static target. */
Vector seek (Vector position, Vector target, float max_speed) {
	return (target - position).Normalize () * max_speed;
}
Vector seek (Vector position, edict_t *target, float max_speed) {
	return seek (position, target->v.origin, max_speed);
}

/** Away from a static target. */
Vector flee (Vector position, Vector target, float max_speed) {
	return (position - target).Normalize () * max_speed;
}
Vector flee (Vector position, edict_t *target, float max_speed) {
	return flee (position, target->v.origin, max_speed);
}

/** Distance between two objects/positions. */
float distance (Vector p1, Vector p2) {
	return (p1 - p2).Length ();
}

/**
 * Where the target will be if they maintain
 * current velocity.
 */
Vector predict (Vector position, edict_t *target) {
	return ((target->v.velocity).Normalize () * distance (position, target->v.origin)) + target->v.origin;
}

/** Toward a moving target. */
Vector pursue (Vector position, edict_t *target, float max_speed, Vector offset) {
	return seek (position, predict (position, target) + offset, max_speed);
}

Vector pursue (Vector position, edict_t *target, float max_speed) {
	return pursue (position, target, max_speed, Vector(0,0,0));
}

/** Away from a moving target. */
Vector evade (Vector position, edict_t *target, float max_speed) {
	return flee (position, predict (position, target), max_speed);
}

float minimum (float n1, float n2) {
	if (n1 < n2)
		return n1;
	else
		return n2;
}

/** Away from local clients. */
Vector separate (edict_t *steered, float distance_threshold, float max_speed) {
	int num_locals = 1;
	Vector destination = steered->v.origin;

	for ( int i = 1; i <= gpGlobals->maxClients; i++) {
		edict_t *client = INDEXENT (i);

		if ((! client) || client->free || client == steered)
			continue;

		bot_t *client_bot = UTIL_GetBotPointer (client);
		bool clientNonBot = (client_bot == NULL);

		if (IsAlive (client)
			//&& FVisible (client->v.origin, steered)
			&& (clientNonBot
				|| ((client->v.velocity).Length () < 10.0)
				|| UTIL_GetBotIndex (steered) < UTIL_GetBotIndex (client)))
		{
			float d = distance (steered->v.origin, client->v.origin);
			d -= BOT_DIAMETER;
			d = clamp (d, 1.0, distance_threshold);
			if (d < distance_threshold) {
				num_locals += 1;
				Vector repulse = (steered->v.origin - client->v.origin).Normalize ();
				repulse = repulse * distance_threshold;
				destination = destination + (repulse / (d * d));
			}
		}
	}

	if (num_locals == 1) // only self
		return VEL_NOCHANGE;

	return seek (steered->v.origin, destination, max_speed);
}

/** Toward next waypoint. */
Vector follow_wp (bot_t *pBot, float max_speed) {
	return seek (pBot->pEdict->v.origin, pBot->wp_origin, max_speed);
}

/**
 * Corrects the current desired
 * velocity to accomodate obstacles.
 */
Vector avoid_obstacles (bot_t *pBot, float max_speed) {
	edict_t *pEdict = pBot->pEdict;

	if (pBot->move_speed > 0.0) {
		if (obstacleJump (pBot)) {
			speak (pBot, "OBSTACLE JUMPING");
			pEdict->v.button |= IN_JUMP;
		}

		if (obstacleDuck (pBot)) {
			speak (pBot, "OBSTACLE DUCKING");
			pEdict->v.button |= IN_DUCK;
		}

		if (obstacleFront (pBot)) {
			speak (pBot, "OBSTACLE FRONT");
			//pBot->desiredVelocity = 
		}
		
		if (obstacleLeft (pBot)) {
			speak (pBot, "OBSTACLE LEFT");
			//pBot->desiredVelocity = 
		}

		if (obstacleRight (pBot)) {
			speak (pBot, "OBSTACLE RIGHT");
			//pBot->desiredVelocity = 
		}
	}

	return pBot->desiredVelocity;
}
