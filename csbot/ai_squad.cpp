/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_squad.cpp,v $
$Revision: 1.10 $
$Date: 2001/05/10 10:00:25 $
$State: Exp $

***********************************

$Log: ai_squad.cpp,v $
Revision 1.10  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.9  2001/05/02 13:09:31  eric
Housekeeping

Revision 1.8  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.7  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

Revision 1.6  2001/04/30 08:06:10  eric
willFire() deals with crouching versus standing

Revision 1.5  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.4  2001/04/29 22:00:20  eric
Source changes for linux compatability

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
#include "ai_squad.h"
#include "ai_squad_func.h"
#include "bot.h"
#include "waypoint.h"

extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern bot_t bots[MAX_BOTS];


squad_t squads[MAX_SQUADS];


/** Initializes the given squad. */
void SquadInit (squad_t *pSquad) {
	pSquad->type = ASSAULT;
	pSquad->goal = PATROL;
	pSquad->wp_current = WP_INVALID;
	pSquad->wp_intermediate = WP_INVALID;
	pSquad->wp_destination = WP_INVALID;
}

/** Returns the team of the squad. */
int squadTeam (squad_t *pSquad) {
	bot_t *member = findRandomSquadMember (pSquad);
	if (member != NULL)
		return UTIL_GetTeam (member->pEdict);
	else
		return -1; // invalid team code
}

/**
 * Returns true iff the given bot and anonymous
 * entity are in the same squad.
 */
bool sameSquad (bot_t *pBot, edict_t *pEdict) {
	squad_t *pSquad = pBot->pSquad;

	if (pSquad == NULL)
		return false;

	for (int i = 0; i < MAX_BOTS; i++) {
		bot_t *bot = &bots[i];
		if (bot->pEdict == pEdict && bot->pSquad == pSquad) {
			return true;
		}
	}

	return false;
}

/**
 * Returns true iff the given two
 * entities are on the same team.
 */
bool sameTeam (edict_t *e1, edict_t *e2) {
	return (UTIL_GetTeam (e1) == UTIL_GetTeam (e2));
}

/**
 * Returns true iff the given bot and
 * squad are on the same team.
 */
bool sameTeam (bot_t *pBot, squad_t *pSquad) {
	if (pBot == NULL || pSquad == NULL)
		return false;

	pBot->bot_team = UTIL_GetTeam (pBot->pEdict);

	return (squadTeam (pSquad) == pBot->bot_team);
}

/** Returns the number of bots in the given squad. */
int squadSize (squad_t *pSquad) {
	if (pSquad == NULL)
		return 0;

	int size = 0;
	for (int i = 0; i < MAX_BOTS; i++) {
		if (bots[i].is_used) {
			if (bots[i].pSquad == pSquad)
				size++;
		}
	}

	return size;
}

/**
 * Returns true iff there is room for
  * another member in the given squad.
 */
bool acceptingMembers (squad_t *pSquad) {
	return (squadSize (pSquad) < MAX_SQUAD_SIZE);
}

/**
 * Returns the squad that the
 * given bot is assigned to.
 */
squad_t *assignSquad (bot_t *pBot) {
	int indices[MAX_SQUADS];

	int count = 0;
	for (int i = 0; i < MAX_SQUADS; i++) {
		if (bots[i].is_used) { // only guaranteed to exist with matching is_used index
			squad_t *pSquad = &squads[i];
			int size = squadSize (pSquad);
			if (sameTeam (pBot, pSquad) && acceptingMembers (pSquad)) {
				for (int s = 0; s < size; s++) { // weight bigger squads
					indices[count] = i;
					count++;
				}
			}
		}
	}

	if (count == 0) // this should never happen because bots should always have a squad
		return NULL;

	int squad_num = RANDOM_LONG (0, count - 1);
/*
	char msg[100];
	sprintf (msg, "squad_num=%i", squad_num);
	speak (pBot, msg);
*/
	return &squads [indices [squad_num]];
}

/**
 * Returns the squad position of the
 * given bot.
 */
SQUAD_POSITION getSquadPosition (bot_t *pBot) {
	squad_t *pSquad = pBot->pSquad;

	if (pSquad == NULL)
		return FIRST;

	int wp_squad = pSquad->wp_current;
	if (! validWaypoint (wp_squad))
		return FIRST;

	Vector v_squad = waypoints[wp_squad].origin;

	float my_d = distance (v_squad, pBot->pEdict->v.origin);

	int num_in_front = 0;
	for (int i = 0; i < MAX_BOTS; i++) {
		bot_t *bot = &bots[i];
		if (bot->is_used && sameSquad (pBot, bot->pEdict)) {
			float mate_d = distance (v_squad, bot->pEdict->v.origin);

			if (mate_d < my_d)
				num_in_front++;
		}
	}

	switch (num_in_front) {
	case 0:
		//speak (pBot, "FIRST");
		return FIRST;
		break;
	case 1:
		//speak (pBot, "SECOND");
		return SECOND;
		break;
	case 2:
		//speak (pBot, "THIRD");
		return THIRD;
		break;
	case 3:
		//speak (pBot, "FOURTH");
		return FOURTH;
		break;
	}

	return FIRST;
}

/**
 * Returns the squad role for the
 * given bot in the given squad.
 */
SQUAD_ROLE getSquadRole (bot_t *pBot) {
/*
	if (pBot->squad_position != NULL) {
		switch (pBot->squad_position) {
		case THIRD:
		case FOURTH:
			return OVERWATCH;
			break;
		default:
		case FIRST:
		case SECOND:
			return MANEUVER;
			break;
		}
	} 
*/
	return MANEUVER;
}

/**
 * Returns a random member of the given
 * squad. Returns NULL if no members are found.
 */
bot_t *findRandomSquadMember (squad_t *pSquad) {
	if (pSquad == NULL)
		return NULL;

	int indices[MAX_BOTS];
	int count = 0;
	for (int i = 0; i < MAX_BOTS; i++) {
		bot_t *pBot = &bots[i];
		if (pBot->pSquad == pSquad) {
			indices[count] = i;
			count++;
		}
	}

	if (count == 0)
		return NULL;

	return &bots [indices [RANDOM_LONG (0, count - 1)]];
}

/**
 * Gives the bot an opportunity to
 * decide to change it's movement style.
 */
bool reacessMovementStyle (bot_t *pBot) {
	if (pBot->movethink_time <= gpGlobals->time) {
		pBot->movethink_time = gpGlobals->time + RANDOM_FLOAT (4.0, 8.0);
		
		// TODO: shift movement style

		return true;
	}

	return false;
}

/**
 * Maintains the bot's understanding of its
 * role within its squad.
 */
bool BotSquad (bot_t *pBot) {
	if (pBot->pSquad == NULL)
		pBot->pSquad = assignSquad (pBot);

	reacessMovementStyle (pBot);

	if (pBot->squad_time <= gpGlobals->time) {
		pBot->squad_time = gpGlobals->time + RANDOM_FLOAT (2.0, 5.0);

		// TODO: consider changing squads

		pBot->squad_position = getSquadPosition (pBot);
		pBot->squad_role = getSquadRole (pBot);

		return true;
	}

	return false;
}

/**
 * Returns the distance to the nearest
 * squadmate of the given bot. Negative
 * one is returned to indicate no other
 * squadmates (other than self) exist
 * or are alive.
 */
float nearestSquadmate (bot_t *pBot) {
	float min_distance = -1.0;
	for (int i = 0; i < MAX_BOTS; i++) {
		bot_t *bot = &bots[i];
		if (bot->is_used
			&& pBot != bot
			&& IsAlive (bot->pEdict)
			&& sameSquad (pBot, bot->pEdict))
		{
			float d = distance (pBot->pEdict->v.origin, bot->pEdict->v.origin);
			if (min_distance == -1.0 || d < min_distance)
				min_distance = d;
		}
	}

	return min_distance;
}
