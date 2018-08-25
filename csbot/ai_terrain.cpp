/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************
Taken from William van der Sterren's
paper entitled "CGF Terrain Specific
AI - Terrain analysis".
	william@botepidemic.com
	www.botepidemic.com/aid/cgf/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_terrain.cpp,v $
$Revision: 1.10 $
$Date: 2001/05/10 10:00:25 $
$State: Exp $

***********************************

$Log: ai_terrain.cpp,v $
Revision 1.10  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.9  2001/05/02 13:09:31  eric
Housekeeping

Revision 1.8  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.7  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

Revision 1.6  2001/04/30 10:44:20  eric
Tactical terrain evaluation fundamentals

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
#include "ai_terrain.h"
#include "waypoint.h"

extern bot_t bots[MAX_BOTS];
extern int num_waypoints;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern VISIBILITY *visibility[MAX_WAYPOINTS];
extern TERRAIN terrain[MAX_WAYPOINTS];


/*
TODO:

(1) These methods assume a fairly even
distribution of waypoints, this may bias
terrain analysis.

(2) Deleted waypoints may be being used
and should be ignored instead.
*/


/**
 * Returns true iff the target waypoint
 * is visible from the source waypoint.
 */
bool visibleFrom (int wp_target, int wp_src) {
	VISIBILITY *v = visibility[wp_src];
	short int num = 0;

	while (v != NULL) {
		if (v->index == wp_target)
			return true;

		v = v->next;
	}

	return false;
}

/**
 * Returns the travelling distance
 * from the source waypoint to the
 * destination waypoint.
 */
float terrainDistance (WP_INDEX src, WP_INDEX dest) {
	return WaypointDistanceFromTo (src, dest);
}

/**
 * Returns the straight-line distance
 * between the given two waypoints.
 */
float directDistance (WP_INDEX src, WP_INDEX dest) {
	return distance (waypoints[src].origin, waypoints[dest].origin);
}

/** Creates an influence with the given value. */
INFLUENCE createInfluence (float v) {
	return clamp (v, 0.0, 1.0); // [0,1]
}

/**
 * Returns the average distance from
 * the given waypoint to all visible waypoints.
 */
float avgVisDistance (WP_INDEX wp) {
	float d_avg = 0.0;

	unsigned int count = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (i, wp)) {
			count++;
			float d = directDistance (wp, i);
			/*
			d_avg *= ((count - 1) / count);
			d_avg += (d * (1 / count));
			*/
			d_avg += d;
		}
    }

	//return d_avg;
	return (d_avg / count);
}

/**
 * Determines the speed ratio
 * [0,1] at the given waypoint.
 * One indicates full speed. Zero
 * indicates no movement whatsoever.
 */
float speedRatio (WP_INDEX wp) {
	float speed = 1.0;

	if (waypoints[wp].flags & W_FL_DOOR)
		speed *= 0.8;

	if (waypoints[wp].flags & W_FL_CROUCH)
		speed *= 0.6;

	if (waypoints[wp].flags & W_FL_LIFT)
		speed *= 0.5;

	if (waypoints[wp].flags & W_FL_JUMP)
		speed *= 0.4;

	if (waypoints[wp].flags & W_FL_LADDER)
		speed *= 0.2;

	return clamp (speed, 0.0, 1.0); // [0,1]
}

/**
 * Returns the amount of relative light
 * at the given waypoint bound to the
 * range [0,1].
 */
float lightLevel (WP_INDEX wp) {
	return 1.0; // TODO
}

/**
 * Returns a value bound to [0,1]
 * representing the level of aggregate
 * advantage for the given paramater
 * distribution.
 */
float distribution (unsigned int advantage, unsigned int disadvantage, unsigned int neither) {
	float influence = 0.0;
	unsigned int total = advantage + disadvantage + neither;

	if (total > 0)
		influence = ((0.0 * disadvantage) + (0.5 * neither) + (1.0 * advantage)) / total;

	return clamp (influence, 0.0, 1.0);
}

/**
 * Zero denotes no access advantage. One indicates
 * that all visible waypoints have trouble accessing
 * THIS waypoint. An influence value of half indicates
 * that half of the visible waypoints have
 * access disadvantage.
 */
INFLUENCE InfluenceAccess (WP_INDEX wp) {
	static const negligible_distance = WAYPOINT_SPACING;

	unsigned int advantage = 0;
	unsigned int disadvantage = 0;
	unsigned int neither = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (i, wp)) {
			float from_here = directDistance (wp, i);
			float from_there = terrainDistance (i, wp);
			float diff = from_here - from_there;
			if (abs (diff) > negligible_distance) {
				if (diff < 0.0)
					advantage++;
				else if (diff >= 0.0)
					disadvantage++;
			} else {
				neither++;
			}
		}
    }

	float access = distribution (advantage, disadvantage, neither);
	return createInfluence (access);
}

/**
 * Indicates how well cover is accessible
 * from potential threats at all visible waypoints.
 */
INFLUENCE InfluenceCover (WP_INDEX wp) {
	unsigned int cover = 0;
	unsigned int no_cover = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (wp, i)) {
			int num_cover = coverCount (wp, i);

			if (num_cover > 0)
				cover++;
			else
				no_cover++;
		}
    }

	float my_cover = distribution (cover, no_cover, 0);
	return createInfluence (my_cover);
}

/**
 * Zero denotes that all visible waypoints are
 * significantly less lit than THIS waypoint.
 * One indicates that all visible waypoints are
 * significantly more illuminated than THIS
 * waypoint. A value of half indicates that there
 * is no significant visibility advantage or
 * disadvantage for THIS waypoint.
 */
INFLUENCE InfluenceVisibility (WP_INDEX wp) {
	static const negligible_lighting = 0.5; // TODO: check this konstant

	float light_here = lightLevel (wp);

	unsigned int darker = 0;
	unsigned int lighter = 0;
	unsigned int neither = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (i, wp)) {
			float light_there = lightLevel (i);
			float diff = light_here - light_there;
			if (abs (diff) > negligible_lighting) {
				if (diff < 0.0)
					darker++;
				else if (diff >= 0.0)
					lighter++;
			} else {
				neither++;
			}
		}
    }

	float visibility = distribution (darker, lighter, neither);
	return createInfluence (visibility);
}

/**
 * Indicates how inaccessible cover is to
 * potential threats at all visible waypoints.
 */
INFLUENCE InfluenceNoCover (WP_INDEX wp) {
	unsigned int no_cover = 0;
	unsigned int cover = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (i, wp)) {
			int num_cover = coverCount (i, wp);

			if (num_cover > 0)
				cover++;
			else
				no_cover++;
		}
    }

	float their_cover = distribution (no_cover, cover, 0);
	return createInfluence (their_cover);
}

/**
 * Zero indicates that all visible waypoints
 * have less restricted movement than THIS waypoint.
 * One indicates that all visible waypoints have
 * more restricted movement than THIS waypoint.
 * The gray area in between is a spectrum of advantage.
 */
INFLUENCE InfluenceSpeed (WP_INDEX wp) {
	static const negligible_speed = 0.3; // TODO: check this konstant

	float speed_here = speedRatio (wp);

	unsigned int faster = 0;
	unsigned int slower = 0;
	unsigned int neither = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (i, wp)) {
			float speed_there = speedRatio (i);
			float diff = speed_here - speed_there;
			if (abs (diff) > negligible_speed) {
				if (diff > 0.0)
					faster++;
				else if (diff <= 0.0)
					slower++;
			} else {
				neither++;
			}
		}
    }

	float speed = distribution (faster, slower, neither);
	return createInfluence (speed);
}

/**
 * Indicates how accessible access to this
 * waypoint is without being observed.
 */
INFLUENCE InfluenceSurprise (WP_INDEX wp) {	
	unsigned int in_open = 0;
	unsigned int surprisable = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (i == wp)
			continue;

		PATH *pPath = NULL;
		int wp_adjacent = WP_INVALID;
		int pI;
		while ((wp_adjacent = WaypointFindPath (&pPath, &pI, i)) != -1) {
			if (wp_adjacent == wp)
				continue;

			if (isPath (wp_adjacent, wp)) {
				if (!visibleFrom (i, wp))
					surprisable++;
				else
					in_open++;
			}
		}
	}

	float surprise = distribution (in_open, surprisable, 0);
	return createInfluence (surprise);
}

/**
 * Zero denotes that this waypoint is significantly
 * than all visible waypoints. One denotes this
 * waypoint is siginificantly more elevated than all
 * visible waypoints. Half denotes that this waypoint
 * is significantly neither above nor below all
 * visible waypoints.
 */
INFLUENCE InfluenceElevation (WP_INDEX wp) {
	static const float significance = PLAYER_HEIGHT;

	float wp_elevation = waypoints[wp].origin.z;

	unsigned int above = 0;
	unsigned int below = 0;
	unsigned int neither = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (i, wp)) {
			float vis_elevation = waypoints[i].origin.z;
			float diff = wp_elevation - vis_elevation;
			if (abs (diff) > significance) {
				if (diff > 0.0)
					above++;
				else if (diff <= 0.0)
					below++;
			} else {
				neither++;
			}
		}
    }

	float elevation = distribution (above, below, neither);
	return createInfluence (elevation);
}

/** Returns TRUE iff the given terrain is valid. */
bool validTerrain (TERRAIN *t) {
	return (t != NULL && validWaypoint (t->index));
}

/**
 * Evaluates the tactical value of
 * the given terrain for an ambush.
 * The result is bound to [0,100] with
 * greater values being better.
 */
float evaluateAmbush (TERRAIN *t) {
	if (! validTerrain (t))
		return 0.0;

	return (
		(t->access * 100.0) +
		(t->cover * 1.0) +
		(t->visibility * 100.0) +
		(t->no_cover * 10.0) +
		(t->speed * 100.0) +
		(t->surprise * 100.0) +
		(t->elevation * 10.0)
	);
}

/**
 * Evaluates the tactical value of
 * the given terrain for sniping.
 * The result is bound to [0,100] with
 * greater values being better.
 */
float evaluateSniping (TERRAIN *t) {
	if (! validTerrain (t))
		return 0.0;

	float distance = clamp (t->avg_v_distance, 0.0, MAX_DISTANCE);
	distance /= MAX_DISTANCE; // [0,1]

	return (
		(distance * 100.0) +
		(t->access * 10.0) +
		(t->cover * 10.0) +
		(t->visibility * 10.0) +
		(t->no_cover * 1.0) +
		(t->speed * 10.0) +
		(t->surprise * 100.0) +
		(t->elevation * 10.0)
	);
}

/**
 * Evaluates the tactical value of
 * the given terrain for stealth.
 * The result is bound to [0,100] with
 * greater values being better.
 */
float evaluateStealth (TERRAIN *t) {
	if (! validTerrain (t))
		return 0.0;

	return (
		(t->access * 10.0) +
		(t->cover * 10.0) +
		(t->visibility * 100.0) +
		(t->no_cover * 10.0) +
		(t->speed * 10.0) +
		(t->surprise * 100.0) +
		(t->elevation * 10.0)
	);
}

/**
 * Evaluates the tactical value of
 * the given terrain for travelling.
 * The result is bound to [0,100] with
 * greater values being better.
 */
float evaluateTravel (TERRAIN *t) {
	if (! validTerrain (t))
		return 0.0;

	return (
		(t->access * 10.0) +
		(t->cover * 10.0) +
		(t->visibility * 10.0) +
		(t->no_cover * 10.0) +
		(t->speed * 10.0) +
		(t->surprise * 10.0) +
		(t->elevation * 10.0)
	);
}

/** Initializes the terrain at the given waypoint. */
void terrainInit (TERRAIN *t, WP_INDEX wp) {
	t->index			= wp;
	t->avg_v_distance	= avgVisDistance (wp);
	t->light_level		= lightLevel (wp);
	t->movement_speed	= speedRatio (wp);

	t->access			= InfluenceAccess (wp);
	t->cover			= InfluenceCover (wp);
	t->visibility		= InfluenceVisibility (wp);
	t->no_cover			= InfluenceNoCover (wp);
	t->speed			= InfluenceSpeed (wp);
	t->surprise			= InfluenceSurprise (wp);
	t->elevation		= InfluenceElevation (wp);

	t->ambush			= evaluateAmbush (t);
	t->sniping			= evaluateSniping (t);
	t->stealth			= evaluateStealth (t);
	t->travel			= evaluateTravel (t);
}

/** Resets the terrain info. */
void terrainReset (TERRAIN *t) {
	t->index			= WP_INVALID;
	t->avg_v_distance	= 0.0;
	t->light_level		= 1.0;
	t->movement_speed	= 1.0;

	t->access			= 0.0;
	t->cover			= 0.0;
	t->visibility		= 0.0;
	t->no_cover			= 0.0;
	t->speed			= 0.0;
	t->surprise			= 0.0;
	t->elevation		= 0.0;

	t->ambush			= 0.0;
	t->sniping			= 0.0;
	t->stealth			= 0.0;
	t->travel			= 0.0;
}

void TerrainInitAll (void) {
	for (int t = 0; t < num_waypoints; t++) {
		terrainReset (&terrain[t]);
		//terrainInit (&terrain[t], t);
	}
}

/** Prints terrain information about the given waypoint index. */
void WaypointPrintTerrainInfo (edict_t *pEntity, int wp) {
   char msg[80];

   TERRAIN *t = &terrain[wp];

   sprintf (msg,"Light level: %f\n", t->light_level);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Avg D: %f\n", t->avg_v_distance);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Speed: %f\n", t->movement_speed);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Influence access: %f\n", t->access);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Influence cover: %f\n", t->cover);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Influence visibility: %f\n", t->visibility);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Influence no-cover: %f\n", t->no_cover);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Influence speed: %f\n", t->speed);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Influence surprise: %f\n", t->surprise);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Influence elevation: %f\n", t->elevation);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Ambush value: %f\n", t->ambush);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Sniping value: %f\n", t->sniping);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Stealth value: %f\n", t->stealth);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Travel value: %f\n", t->travel);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);
}

/**
 * Returns true iff another teammate
 * has already claimed residency of this
 * tactical waypoint.
 */
bool wpClaimedTactical (bot_t *pBot, int wp) {
	for (int i = 0; i < MAX_BOTS; i++) {
		bot_t *bot = &bots[i];
		if (bot->is_used
			&& IsAlive (bot->pEdict)
			&& pBot != bot
			&& sameTeam (pBot->pEdict, bot->pEdict))
		{
			if (bot->wp_tactical == wp && bot->tactical_time > gpGlobals->time)
				return true;
		}
	}

	return false;
}

/**
 * Returns the number of potential cover
 * waypoints from the given threat.
 */
int coverCount (int wp_src, int wp_threat) {
	unsigned int count = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (!visibleFrom (i, wp_threat)
			&& i != wp_threat
			&& terrainDistance (wp_src, i) < REACHABLE_RANGE)
		{
			count++;
		}
    }

	return count;
}

/**
 * Sets the given result int list to the
 * closest waypoints having cover from the given
 * threat waypoint. Returns the number of
 * elements returned in result.
 */
int WaypointCover (bot_t *pBot, int wp_threat, float range, int result[], unsigned int result_size) {
	int wp_me = WaypointFindNearest (pBot->pEdict, REACHABLE_RANGE);

	unsigned int count = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (!visibleFrom (i, wp_threat)
			&& i != wp_threat
			&& !wpClaimedTactical (pBot, i)
			&& terrainDistance (wp_me, i) < range)
		{
			if (count < result_size) {
				result[count] = i;
				count++;
			} else {
				for (unsigned int j = 0; j < result_size; j++) {
					if (terrainDistance (wp_me, i) < terrainDistance (wp_me, result[j])) {
						result[j] = i; // i better
						break;
					}
				}
			}
		}
    }

	return count;
}

/**
 * Sets the given result int list to
 * the choke points for the given source.
 * Returns the number of elements returned.
 */
int WaypointChoke (int wp_src, int result[], unsigned int result_size) {
	return 0; // TODO
}

/**
 * Sets the given result int list to the
 * best waypoints for ambushing the given
 * target. Returns the number of elements
 * returned.
 */
int WaypointAmbush (bot_t *pBot, int wp_target, float range, int result[], unsigned int result_size) {
	unsigned int count = 0;
	for (int i = 0; i < num_waypoints; i++) {
		if (visibleFrom (wp_target, i)
			&& i != wp_target
			&& !wpClaimedTactical (pBot, i)
			&& terrainDistance (pBot->wp_current, i) < range)
		{
			if (count < result_size) {
				result[count] = i;
				count++;
			} else {
				for (unsigned int j = 0; j < result_size; j++) {
					if (terrain[result[j]].ambush < terrain[i].ambush) {
						result[j] = i; // i better
						break;
					}
				}
			}
		}
    }

	return count;
}

