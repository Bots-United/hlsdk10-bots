/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_objective.cpp,v $
$Revision: 1.10 $
$Date: 2001/05/10 10:00:25 $
$State: Exp $

***********************************

$Log: ai_objective.cpp,v $
Revision 1.10  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.9  2001/05/06 12:53:39  eric
Prepping windows build for initial release

Revision 1.8  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.7  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

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
#include "ai_squad_func.h"
#include "bot_func.h"
#include "waypoint.h"

extern int num_bots;
extern bot_t bots[MAX_BOTS];
bool bomb_planted = false;
bool bomb_on_ground = false;


/** Called when the bomb has just been placed. */
void bomb_is_planted (void) {
	if (! bomb_planted)
		resetAllSquadsNav ();
	bomb_planted = true;
}

/** Called when the bomb has been dropped. */
void bomb_dropped (void) {
	if (! bomb_on_ground)
		resetAllSquadsNav ();
	bomb_on_ground = true;
}

/** Called when the bomb has been picked up. */
void bomb_picked_up (void) {
	if (bomb_on_ground)
		resetAllSquadsNav ();
	bomb_on_ground = false;
}

/**
 * Returns the dropped bomb entity, or
 * NULL if one does not exist.
 */
edict_t *droppedBomb (void) {
	edict_t *pent = NULL;

	while ((pent = UTIL_FindEntityByClassname (pent, "weapon_c4")) != NULL) {
		if (isDroppedC4 (pent)) {
			return pent;
		}
	}

	return NULL;
}

/**
 * Returns the planted bomb entity, or
 * NULL if one does not exist.
 */
edict_t *plantedBomb (void) {
	edict_t *pent = NULL;

	while ((pent = UTIL_FindEntityByClassname (pent, "grenade")) != NULL) {
		if (isPlantedC4 (pent)) {
			return pent;
		}
	}

	return NULL;
}

/** Determines the status of the bomb. */
void check_bomb (void) {
	if (plantedBomb () != NULL)
		bomb_is_planted ();

	if (droppedBomb () != NULL)
		bomb_dropped ();
	else
		bomb_picked_up ();
}

/** Returns true iff the given entity is a func_ entity. */
bool isFunc (edict_t *pent) {
	return (! strncmp("func_", STRING(pent->v.classname), strlen ("func_")));
}

/** Returns true iff the given entity is the C4 lying on the ground. */
bool isDroppedC4 (edict_t *pent) {
	return (isWeapon (pent) && !strcmp ("weapon_c4", STRING (pent->v.classname)));
}

/** Returns true iff the given entity is C4 that has been planted. */
bool isPlantedC4 (edict_t *pent) {
	return ((! strcmp ("grenade", STRING (pent->v.classname)))
		&& (FStrEq (STRING (pent->v.model), "models/w_c4.mdl")));
}

/** Returns true iff the given entity is a bomb site. */
bool isBombSite (edict_t *pent) {
	return (! strcmp ("func_bomb_target", STRING (pent->v.classname)));
}

/**
 * Returns true iff a teammate of the
 * given bot is currently defusing the bomb.
 */
bool teamIsDefusing (bot_t *pBot) {
	pBot->bot_team = UTIL_GetTeam (pBot->pEdict);

	for (int i = 0; i < MAX_BOTS; i++) {
		if (bots[i].is_used
			&& pBot->bot_team == UTIL_GetTeam (bots[i].pEdict)
			&& bots[i].defuse_bomb_time > gpGlobals->time
			&& pBot->pEdict != bots[i].pEdict
			&& IsAlive (bots[i].pEdict))
		{
			return true;
		}
	}

	// TODO: are human clients defusing? check for that here

	return false;
}

/**
 * Returns the general map objective for
 * the given bot.
 */
OBJECTIVE objective (bot_t *pBot) {
	edict_t *pent = NULL;

	pBot->bot_team = UTIL_GetTeam (pBot->pEdict);

	while ((pent = UTIL_FindEntityByClassname (pent, "hostage_entity")) != NULL) {
		if (pBot->bot_team == TERRORIST)
			return HOSTAGE;
		else
			return RESCUE;
	}

	pent = NULL;

	while ((pent = UTIL_FindEntityByClassname (pent, "func_bomb_target")) != NULL) {
		if (pBot->bot_team == TERRORIST)
			return BOMB;
		else
			return DEFUSE;
	}

	pent = NULL;

	while ((pent = UTIL_FindEntityByClassname (pent, "func_vip_safetyzone")) != NULL) {
		if (pBot->bot_team == TERRORIST)
			return ASSASINATE;
		else
			return ESCORT;
	}

	return UNKNOWN;
}

/** Changes the current movement style. */
void setMovementStyle (bot_t *pBot, MOVEMENT style, float duration) {
	pBot->movement_style = style;
	pBot->movethink_time = gpGlobals->time + duration;
}

/**
 * Gives the bot a chance to decide if
 * she would like to change navigational
 * goals.
 */
void reacessSquadGoal (squad_t *pSquad) {
	SQUAD_GOAL new_goal = pSquad->goal;
	int new_goto = pSquad->wp_destination;

	bot_t *pBot = findRandomSquadMember (pSquad);

	edict_t *droppedB = droppedBomb ();
	edict_t *plantedB = plantedBomb ();

	switch (objective (pBot)) {
	case BOMB:
		if (droppedB != NULL) {		
			new_goal = X_GOTO;
			new_goto = WaypointFindNearest (getOrigin (droppedB), droppedB, REACHABLE_RANGE);
			//speak (pBot, "GETTING BOMB FROM GROUND");
		} else if (plantedB == NULL) {
			new_goal = BOMB_SET;
			//speak (pBot, "GOING TO BOMBSITE");
/*		} else if (plantedB != NULL) {
			new_goal = X_DEFEND;
			new_goto = WaypointFindNearest (getOrigin (plantedB), plantedB, REACHABLE_RANGE);
			//speak (pBot, "DEFENDING PLANTED BOMB");*/
		} else {
			new_goal = PATROL;
		}
		break;
	case DEFUSE:
		if (plantedB != NULL) {
			new_goal = X_GOTO;
			new_goto = WaypointFindNearest (getOrigin (plantedB), plantedB, REACHABLE_RANGE);
			//speak (pBot, "GOING TO PLANTED BOMB");
/*		} else if (droppedB != NULL) {
			new_goal = X_DEFEND;
			new_goto = WaypointFindNearest (getOrigin (droppedB), droppedB, REACHABLE_RANGE);
			//speak (pBot, "DEFENDING DROPPED BOMB");*/
		} else {
			// TODO: some squads should rush to a bomb site and defend it
			new_goal = PATROL;
		}
		break;
	case RESCUE:
		new_goal = HOSTAGE_GET;
		break;
	case ASSASINATE://TODO
	case ESCORT://TODO
	case HOSTAGE://TODO
	default:
		break;
	}

	pSquad->goal = new_goal;
	pSquad->wp_destination = new_goto;
}

/**
 * Returns an intermediate waypoint. Not guaranteed
 * to be valid or reachable given the current
 * destination.
 */
int WaypointIntermediate (squad_t *pSquad) {
	bot_t *pBot = findRandomSquadMember (pSquad);
	pBot->bot_team = UTIL_GetTeam (pBot->pEdict);

	if (plantedBomb () != NULL && pBot->bot_team == COUNTER)
		return WP_INVALID;
	else if (droppedBomb () != NULL && pBot->bot_team == TERRORIST)
		return WP_INVALID;
	else
		return WaypointFindRandomGoal (pBot->pEdict, pBot->bot_team);
}

/**
 * Returns the destination waypoint for the
 * given bot, taking into account its current
 * squad goal.
 */
int WaypointByObjective (squad_t *pSquad) {
	bot_t *pBot = findRandomSquadMember (pSquad);

	int wp_flag = 0;

	switch (pSquad->goal) {
	case BOMB_DEFUSE:
	case BOMB_SET:
		wp_flag = W_FL_BOMB_SITE;
		break;
	case X_DEFEND:
	case X_GOTO:
		return pSquad->wp_destination;
		break;
	case HOSTAGE_GET: // TODO
	case PATROL:
	default:
		wp_flag = 0;
		break;
	}

	return WaypointFindRandomGoal (pBot->pEdict, wp_flag);
}
