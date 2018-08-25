/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_squad.h,v $
$Revision: 1.4 $
$Date: 2001/04/30 08:06:10 $
$State: Exp $

***********************************

$Log: ai_squad.h,v $
Revision 1.4  2001/04/30 08:06:10  eric
willFire() deals with crouching versus standing

Revision 1.3  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef AI_SQUAD_H
#define AI_SQUAD_H


const int MAX_SQUADS = 32; // should always equals MAX_BOTS
const int MAX_SQUAD_SIZE = 4;
const float MIN_SQUAD_DISTANCE = 512.0;

typedef enum
{
	BOMB_DEFUSE,
	BOMB_SET,
	HOSTAGE_GET,
	X_DEFEND,
	X_GOTO,
	PATROL
} SQUAD_GOAL;

typedef enum
{
	FIRST,
	SECOND,
	THIRD,
	FOURTH
} SQUAD_POSITION;

typedef enum
{
	MANEUVER,
	OVERWATCH
} SQUAD_ROLE;

typedef enum
{
	ASSAULT,
	SUPPORT,
	SECURITY
} SQUAD_TYPE;

typedef struct
{
	SQUAD_TYPE type;
	SQUAD_GOAL goal;
	int wp_current;
	int wp_intermediate;
	int wp_destination;
} squad_t;


#endif // AI_SQUAD_H
