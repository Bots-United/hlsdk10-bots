/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_tactical.cpp,v $
$Revision: 1.5 $
$Date: 2001/05/10 10:00:25 $
$State: Exp $

***********************************

$Log: ai_tactical.cpp,v $
Revision 1.5  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.4  2001/05/03 19:35:16  eric
CGF-style aiming

Revision 1.3  2001/05/02 13:09:31  eric
Housekeeping

Revision 1.2  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.1  2001/04/30 13:39:21  eric
Primitive cover seeking behavior


**********************************/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "ai.h"
#include "ai_behavior.h"
#include "ai_terrain.h"
#include "bot.h"

extern WAYPOINT waypoints[MAX_WAYPOINTS];

/**
 * Called to indicate that the bot has
 * been issued an order to ambush and/or
 * defend her squad's current destination
 * waypoint.
 */
void ambush (bot_t *pBot) {
	static const unsigned int WP_NUM = 8;
	static const float AMBUSH_RANGE = 512.0;

	if (pBot->tactical_time <= gpGlobals->time) {
		int wp_choke[WP_NUM];
		int num_chokes = WaypointChoke (pBot->pSquad->wp_destination, wp_choke, WP_NUM);
		if (num_chokes <= 0)
			return; // no choke points found

		int wp_ambush[WP_NUM];
		int num_ambushes = WaypointAmbush (pBot, wp_choke[RANDOM_LONG (0, num_chokes - 1)], AMBUSH_RANGE, wp_ambush, WP_NUM);
		if (num_ambushes > 0) {
			pBot->tactical_time = gpGlobals->time + RANDOM_FLOAT (20.0, 60.0);
			pBot->wp_tactical = wp_ambush[RANDOM_LONG (0, num_ambushes - 1)];
		}
	}
}

/**
 * Called to indicate the bot desires
 * to find cover from its current threat.
 */
void cover (bot_t *pBot) {
	static const unsigned int WP_NUM = 8;

	if (pBot->tactical_time <= gpGlobals->time) {
		pBot->tactical_time = gpGlobals->time + RANDOM_FLOAT (1.0, 2.0);

		int wp_cover[WP_NUM];

		if (pBot->threat == NULL)
			return;

		int wp_threat = WaypointFindNearest (pBot->threat, REACHABLE_RANGE);

		float range = distance (waypoints[wp_threat].origin, pBot->pEdict->v.origin);
		range /= 3.0; // look for cover up to one-third the distance to the threat
		range = clamp (range, REACHABLE_RANGE, MAX_DISTANCE);

		int count = WaypointCover (pBot, wp_threat, range, wp_cover, WP_NUM);
		if (count > 0) {
			pBot->wp_tactical = wp_cover[RANDOM_LONG (0, count - 1)];
			if (! randomize (20))
				radio (pBot, RANDOM_FLOAT (0.0, 0.1), "1", "1"); // cover me

			/*
			char msg[100];
			sprintf (msg, "I am seeking cover! WP=%i", pBot->wp_tactical);
			speak (pBot, msg);
			*/
		} else {
			pBot->wp_tactical = WP_INVALID; // invalidate

			//speak (pBot, "I cannot find cover!");
		}
	}
}
