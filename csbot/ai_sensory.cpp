/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************
Adapted from William van der Sterren's
paper entitled "CGF bot vision,
tracking and aiming".
	william@botepidemic.com
	www.botepidemic.com/aid/cgf/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_sensory.cpp,v $
$Revision: 1.13 $
$Date: 2001/05/06 12:53:39 $
$State: Exp $

***********************************

$Log: ai_sensory.cpp,v $
Revision 1.13  2001/05/06 12:53:39  eric
Prepping windows build for initial release

Revision 1.12  2001/05/05 14:42:32  eric
Minor vision improvements

Revision 1.11  2001/05/05 10:29:14  eric
Fixed install and find_threat() bugs

Revision 1.10  2001/05/04 08:18:17  eric
CGF-style vision stubs

Revision 1.9  2001/05/03 19:35:16  eric
CGF-style aiming

Revision 1.8  2001/05/02 13:09:31  eric
Housekeeping

Revision 1.7  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.6  2001/04/30 08:06:10  eric
willFire() deals with crouching versus standing

Revision 1.5  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

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
#include "ai_squad_func.h"
#include "ai_terrain.h"
#include "bot_weapons.h"

extern bool b_observer_mode;
extern bot_t bots[MAX_BOTS];


/**
 * Returns the speed percentage of
 * maximum the given entity is travelling.
 * Assumes the given entity is a client.
 */
float percentSpeed (edict_t *pEdict) {
	static float MAX_SPEED = 260.0; // speed with knife

	if (pEdict == NULL)
		return 0.0;

	float speed = (pEdict->v.velocity).Length ();
	return clamp (speed, 0.0, MAX_SPEED) / MAX_SPEED; // [0,1]
}

/**
 * Returns the scaling factor that should
 * be used when determining how off from
 * the desired aiming location the bot will
 * actually face.
 *<ul>
 * Considers the following factors:
 * <li> velocity of target (aim degrades when target moves)
 * <li> velocity of gunner (aim degrades when gunner moving)
 * <li> tracking time
 * <li> poor visibility conditions (smoke grenades, glass, grates, fences)
 *</ul>
 */
float aimDeviationScale (bot_t *pBot, bool in_sight) { // 0.01
	// poor visibility conditions take affect indirectly via in_sight

	static float WORST_DECAY = -0.01;
	static float BEST_DECAY = -0.1; // TODO: test this

	float skill = (abs (pBot->bot_skill - 100.0) / 100.0); // [0,1]
	float d = BEST_DECAY + (abs (BEST_DECAY - WORST_DECAY) * skill);

	if (in_sight) {
		// target velocity TODO: make relative to gunner velocity
		d += (percentSpeed (pBot->threat) / 20.0); // [0,0.05]

		// gunner velocity
		d += (percentSpeed (pBot->pEdict) / 20.0); // [0,0.05]

		d = clamp (d, BEST_DECAY, WORST_DECAY); // in sight, so improve no matter what	
	} else {
		d = 0.01; // not in sight, so deteriate
	}

	pBot->aim_deviation_scale = clamp (pBot->aim_deviation_scale + d, 0.0, 2.0);
	return pBot->aim_deviation_scale;
}

/** Returns the initial aiming deviation. */
Vector aimDeviationInitial (bot_t *pBot) {
	float d = distance (pBot->threat->v.origin, pBot->pEdict->v.origin);
	float scale = clamp (d / 1000.0, 0.1, 1.0);

	float max_horizontal_deviance = 200.0;
	float max_vertical_deviance = 120.0;

	float d_x = RANDOM_FLOAT (-max_horizontal_deviance, max_horizontal_deviance);
	float d_y = RANDOM_FLOAT (-max_horizontal_deviance, max_horizontal_deviance);
	float d_z = RANDOM_FLOAT (-max_vertical_deviance, max_vertical_deviance);

	return Vector (d_x, d_y, d_z) * scale;
}

/**
 * Returns true iff the listener is
 * able to hear the target.
 *<p>
 * Adapted from TEAMbot 4.5 by
 * Alistair Stewart.
 */
bool hear (edict_t *listener, edict_t *target) {
	static bool check_footsteps = true;
	static bool footsteps_on = true;

	if (check_footsteps) {
		check_footsteps = false;
		footsteps_on = CVAR_GET_FLOAT ("mp_footsteps") > 0.0;
	}

	float volume = 0.0;
	float sensitivity = 1.0;

	if (target->v.button & IN_ATTACK)
		volume = 1500.0;
	else if (footsteps_on && (target->v.velocity.Length2D () > 220.0))
		volume = 1000.0;
	else if ((target->v.button & IN_RELOAD) || (target->v.button & IN_USE))
		volume = 750.0;

	if (footsteps_on && (listener->v.velocity.Length2D () > 220.0))
		sensitivity *= (3 / 4);
	if (listener->v.button & IN_ATTACK)
		sensitivity *= (2 / 3);
	if ((listener->v.button & IN_RELOAD) || (listener->v.button & IN_USE))
		sensitivity *= (5 / 6);

	// TODO: add falling & check for silencer
	// TODO: ladders

	if ((sensitivity * volume) > distance (target->v.origin, listener->v.origin))
		return false;
	else
		return true;
}

/**
 * Returns true if the bot is currently
 * getting hit with bullets.
 */
bool beingHit (bot_t *pBot) {
	return false; // TODO
}

/**
 * Returns the percentage of the target
 * which is visible to the bot. Takes into
 * account if the target is crouching.
 */
float percentVisible (bot_t *pBot, edict_t *target) {
	return 1.0; // TODO
}

/** Returns the percentage illumination on the target. */
float percentLit (edict_t *target) {
	return 1.0; // TODO
}

/**
 * Returns how many degrees off the entity's
 * center of vision the given point lies.
 */
float fovAngle (Vector *pOrigin, edict_t *pEdict) {
	UTIL_MakeVectors (pEdict->v.angles);

	Vector2D vec2LOS = (*pOrigin - pEdict->v.origin).Make2D ();
	vec2LOS = vec2LOS.Normalize ();

	float cosine = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D ());
	cosine = clamp (cosine, -1.0, 1.0);
	
	float fov = abs (cosine - 1.0); // [0,2]
	fov *= 90.0; // [0,180]

	return fov;
}

/**
 * Returns true if the target passes
 * straightforward visiblity checks.
 */
bool inSight (bot_t *pBot, edict_t *target) {
	return (FVisible (target->v.origin, pBot->pEdict) && FInViewCone (&(target->v.origin), pBot->pEdict));
}

/**
 * Returns true iff the target
 * is seen by the looker.
 *<ul>
 * Takes into account the following factors:
 * <li> reduce vision when being hit
 * <li> percent target visible
 * <li> distance to target
 * <li> FOV angle (periphery versus center) [TODO: consider zooming]
 * <li> percent lit [TODO: NVG should eliminate penalty]
 * <li> velocity of target
 * <li> velocity of looker
 * <li> bot's overall skill
 * <li> was the target already seen?
 * <li> target opening fire without suppressor?
 * <li> poor visibility conditions (smoke grenades, glass, grates, fences)
 *</ul>
 */
bool spotted (bot_t *pBot, edict_t *target) {
	// TODO: incorporate CGF-style vision

	if (pBot == NULL || pBot->pEdict == NULL || target == NULL)
		return false;

	return inSight (pBot, target);
/*
	edict_t *pEdict = pBot->pEdict;

	if (! FVisible (target->v.origin, pEdict))
		return false;

	if (target == pBot->threat)
		return true;

	float fov = fovAngle (&(target->v.origin), pEdict);

	static const float FULL_VISION = 135.0;
	float fovPropensity = clamp (fov, 0.0, FULL_VISION);
	fovPropensity /= FULL_VISION; // [0,1]

	static const int TIMED_REPITIONS = 5;
	for (int i = 0; i < TIMED_REPITIONS; i++)
		if (RANDOM_FLOAT (0.0, 1.0) > fovPropensity)
			return false;

	return true;

	//return (fov < 60);
/*
	bool being_hit			= beingHit (pBot); // false better
	float percent_visible	= percentVisible (pBot, target);

	static float VIS_DROPOFF = 2048.0;
	float d	= distance (looker->v.origin, to);
	d		= clamp (d, 0.0, VIS_DROPOFF);
	d		/= VIS_DROPOFF; // [0,1] -- 0 better
	d		= abs (d - 1.0);

	float fov	= fovAngle (pBot, &to); // [0,180]
//	fov			= clamp (fov, 0.0, 90.0); // [0,90]
//	fov			/= 90.0; // [0,1] -- 0 better
//	fov			= abs (fov - 1.0);

	float percent_lit	= percentLit (target);
	float t_speed		= percentSpeed (target);

	float l_speed	= percentSpeed (looker); // [0,1] -- 0 better
	l_speed			= abs (l_speed - 1.0);

	// TODO: skill
	// TODO: already seen?
	// TODO: opening fire without suppressor?
	// TODO: poor visibility conditions
*/
}

/**
 * Returns true iff the only team member
 * who has the target in sight is the given one.
 */
bool onlyInSight (edict_t *looker, edict_t *target) {
	// TODO: this should check human clients as well
	for (int i = 0; i < MAX_BOTS; i++) {
		if (bots[i].is_used
			&& looker != bots[i].pEdict
			&& UTIL_GetTeam (looker) == UTIL_GetTeam (bots[i].pEdict)
			&& inSight (&bots[i], target))
		{
			return false;
		}
	}

	return true;
}

/** Called to remember the current threat. */
void enemyInSight (bot_t *pBot) {
	if (pBot->threat != NULL) {
		switch (pBot->squad_role) {
		case OVERWATCH:
			pBot->threat_giveup_time = RANDOM_FLOAT (3.0, 9.0);
			break;
		default:
		case MANEUVER:
			pBot->threat_giveup_time = RANDOM_FLOAT (2.0, 4.0);
			break;
		}

		pBot->last_seen_threat_location = pBot->threat->v.origin;
		pBot->last_seen_threat_time = gpGlobals->time;
	}
}

/**
 * Returns the most desirable threat available.
 * TODO: should choose closely packed threats
 *     at a distance over nearer solo threats
 * TODO: should consider which direction the
 *     threat is facing
 */
edict_t *find_threat (bot_t *pBot) {
	edict_t *pEdict = pBot->pEdict;

	edict_t *obscured_threat = NULL;

	if (pBot->threat != NULL) {
		if (! IsAlive (pBot->threat)) {
			if (isCurrentRadio (pBot, "3", "2")) // enemy spotted
				radioOverride (pBot, RANDOM_FLOAT (0.5, 1.5), "3", "9"); // enemy down
			else if (pBot->radio_time_down + 5.0 < gpGlobals->time && !randomize (3))
				radio (pBot, RANDOM_FLOAT (0.5, 1.5), "3", "9"); // enemy down

			pBot->threat = NULL;
		}

		if (pBot->last_seen_threat_time + pBot->threat_giveup_time <= gpGlobals->time) {
			obscured_threat = pBot->threat;
			pBot->threat = NULL;
		}
	}

	// TODO: also consider changing threats if desire is great
	if (pBot->threat == NULL || !inSight (pBot, pBot->threat)) {
		float most_desired = 0.0;
		edict_t *chosen_threat = NULL;

		for (int i = 1; i <= gpGlobals->maxClients; i++) {
			edict_t *client = INDEXENT (i);

			if (client == NULL
				|| client->free
				|| client == pEdict
				|| !IsAlive (client)
				|| (b_observer_mode && !(client->v.flags & FL_FAKECLIENT))
				|| UTIL_GetTeam (pEdict) == UTIL_GetTeam (client))
			{
				continue;
			}

			//if (hear (pEdict, client) || inSight (pBot, client)) {
			if (spotted (pBot, client)) {
				if (pBot->radio_time + 10 < gpGlobals->time && onlyInSight (pEdict, client))
					radio (pBot, RANDOM_FLOAT (1.0, 2.0), "3", "2"); // enemy spotted
				
				int wp_bot = WaypointFindNearest (pBot->pEdict, REACHABLE_RANGE);
				int wp_threat = WaypointFindNearest (client, REACHABLE_RANGE);

				float desire = 100.0;
				float d = terrainDistance (wp_threat, wp_bot);
				desire /= (d * d);
				if (desire > most_desired) {
					most_desired = desire;
					chosen_threat = client;
				}
			}
		}

		if (chosen_threat != NULL && chosen_threat != obscured_threat) {
			pBot->threat = chosen_threat;
			if (aimHead (pBot))
				pBot->aim_head = pBot->threat->v.view_ofs * RANDOM_FLOAT (0.5, 1.0);
			else
				pBot->aim_head = Vector (0, 0, 0);
			pBot->aim_deviation = aimDeviationInitial (pBot);
			pBot->aim_deviation_scale = 1.0;
		} else { // keep out of sight threat
			pBot->threat = obscured_threat;
		}
	}

	return pBot->threat;
}

/** Returns the most desirable pickup available. */
edict_t *find_pickup (bot_t *pBot) {
	edict_t *pent = NULL;
	edict_t *pickup = NULL;
	float radius = 256.0;
	float most_desired = 0.0;
	edict_t *pEdict = pBot->pEdict;

	pBot->bot_team = UTIL_GetTeam (pEdict);

	while ((pent = UTIL_FindEntityInSphere (pent, pEdict->v.origin, radius)) != NULL) {
		bool can_pickup = false;

		if (isWeapon (pent) || isPlantedC4 (pent) || isBombSite (pent))
			can_pickup = true;

		if (can_pickup) {
			float desire = 0.0;
			if ((! havePrimaryWeapon (pBot)) && isPrimaryWeapon (pent)) {
				desire = 100.0;
			} else if (pBot->bot_team == TERRORIST && isDroppedC4 (pent)) {
				//speak (pBot, "I SEE DROPPED C4");
				return pent;
			} else if (pBot->bot_team == TERRORIST && isBombSite (pent) && haveWeapon (pBot, CS_WEAPON_C4)) {
				return pent;
			} else if (pBot->bot_team == COUNTER && isPlantedC4 (pent) && !teamIsDefusing (pBot)) {
				return pent;
			} else if (! isWeapon (pent)) {
				continue;
			} else if (! FVisible (pent->v.owner->v.origin, pEdict)) {
				continue;
			}
			// careful here, pent->v.owner->v.origin only necessarily
			// exists because this must be a weapon at this point
			desire /= distance (pEdict->v.origin, pent->v.owner->v.origin);
			if (desire > most_desired) {
				//speak (pBot, "I SEE A GUN I WANT");
				most_desired = desire;
				pickup = pent;
			}
		}
	}

	return pickup;
}

/** Scans the environment, and remembers critical details. */
void scan_environment (bot_t *pBot) {
	pBot->pickup = find_pickup (pBot);

	pBot->threat = find_threat (pBot);

	if (pBot->threat != NULL && inSight (pBot, pBot->threat))
		enemyInSight (pBot);
}
