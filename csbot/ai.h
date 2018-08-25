/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai.h,v $
$Revision: 1.14 $
$Date: 2001/05/11 06:17:22 $
$State: Exp $

***********************************

$Log: ai.h,v $
Revision 1.14  2001/05/11 06:17:22  eric
navigation cleanup

Revision 1.13  2001/05/06 01:48:37  eric
Replaced global constants with enums where appropriate

Revision 1.12  2001/05/05 14:42:32  eric
Minor vision improvements

Revision 1.11  2001/05/04 08:18:17  eric
CGF-style vision stubs

Revision 1.10  2001/05/03 19:35:16  eric
CGF-style aiming

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

Revision 1.3  2001/04/29 04:13:41  eric
Trimmed down bot structure

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef AI_H
#define AI_H

#include "bot.h"

const int TERRORIST = 0;
const int COUNTER = 1;

const float MAX_DISTANCE		= 8192.0; // TODO: should include diagonal in 3D?
const float PLAYER_HEIGHT		= 72.0;
const float BOT_DIAMETER		= 46.0; // approx hypotoneuse of 32x32 triangle
const float WP_TOUCH_DISTANCE	= ((BOT_DIAMETER / 2.0) + 1.0);
const Vector VEL_NOCHANGE		= Vector (0, 0, 0);

// willFire flags (32 bits)
const int CROUCH_CLEAR		= (1<<1);
const int STANDING_CLEAR	= (1<<2);
const int JUMPING_CLEAR		= (1<<3);
const int SUPPRESSIVE		= (1<<4);

/*
BUGS:
(1) seeks when knife attacking more often than should
(2) review buying frequencies with Tyler
(3) review weapon usages with Tyler
(4) be creative in where/how bot plants
(5) use WP_INDEX everywhere
(6) not joining squads well at beginning of first map
(7) smaller squads should merge?
(8) when out of range with current gun, but currently keeps
taking cover, but should run away outright or pick a gun
with the appropriate range
(9) some amount of speed is being lost (check max_speed vs. v.velocity)
*/

// ai_arsenal.cpp
bool usingKnife (bot_t *pBot);
bool suppressionFire (bot_t *pBot);
bool aimHead (bot_t *pBot);
bool inRange (bot_t *pBot);
bool haveAmmo (bot_t *pBot);
bool ammoInClip (bot_t *pBot);
int willFire (bot_t *pBot);
bool openFire (bot_t *pBot);
bool haveWeapon (bot_t *pBot, int weapon);
bool havePrimaryWeapon (bot_t *pBot);
bool haveSecondaryWeapon (bot_t *pBot);
bool isPrimaryWeapon (edict_t *pent);
void changeWeapon (bot_t *pBot);

// ai_econ.cpp
void BotBuy (bot_t *pBot);
bool isBuyMsg (CS_MSG msg);

// ai_move.cpp
Vector clampAngleTo180 (Vector angle);
Vector smoothLook (bot_t *pBot);
Vector smoothVelocity (bot_t *pBot);
bool visibleWaypoint (bot_t *pBot, int waypoint);
bool validWaypoint (int waypoint);
Vector eyePosition (edict_t *pEdict);
Vector lookAhead (bot_t *pBot);
Vector lookThreat (bot_t *pBot);
Vector getOrigin (edict_t *pEdict);
Vector pickupOrigin (bot_t *pBot);
Vector lookPickup (bot_t *pBot);

// ai_objective.cpp
void check_bomb (void);
bool isWeapon (edict_t *pent);
bool isFunc (edict_t *pent);
bool isDroppedC4 (edict_t *pent);
bool isPlantedC4 (edict_t *pent);
bool isBombSite (edict_t *pent);
bool teamIsDefusing (bot_t *pBot);
void setMovementStyle (bot_t *pBot, MOVEMENT style, float duration);
void reacessSquadGoal (squad_t *pSquad);
int WaypointIntermediate (squad_t *pSquad);
int WaypointByObjective (squad_t *pSquad);

// ai_radio.cpp
bool isCurrentRadio (bot_t *pBot, char *radio_type, char *radio_num);
void radioOverride (bot_t *pBot, float delay, char *radio_type, char *radio_num);
void radio (bot_t *pBot, float delay, char *radio_type, char *radio_num);
bool BotRadio (bot_t *pBot);
bool speak (bot_t *pBot, char *msg);

// ai_sensory.cpp
float aimDeviationScale (bot_t *pBot, bool in_sight);
bool inSight (bot_t *pBot, edict_t *target);
bool spotted (bot_t *pBot, edict_t *target);
void scan_environment (bot_t *pBot);

// ai_steering.cpp
Vector seek (Vector position, edict_t *target, float max_speed);
Vector seek (Vector position, Vector target, float max_speed);
float distance (Vector p1, Vector p2);
Vector separate (edict_t *steered, float distance_threshold, float max_speed);
Vector follow_wp (bot_t *pBot, float max_speed);
Vector avoid_obstacles (bot_t *pBot, float max_speed);

// ai_tactical.cpp
void ambush (bot_t *pBot);
void cover (bot_t *pBot);


#endif // AI_H
