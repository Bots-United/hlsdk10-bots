/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_squad_func.h,v $
$Revision: 1.5 $
$Date: 2001/04/30 13:39:21 $
$State: Exp $

***********************************

$Log: ai_squad_func.h,v $
Revision 1.5  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

Revision 1.4  2001/04/30 08:06:10  eric
willFire() deals with crouching versus standing

Revision 1.3  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef AI_SQUAD_FUNC_H
#define AI_SQUAD_FUNC_H

#include "ai_squad.h"


void SquadInit (squad_t *pSquad);
bool sameSquad (bot_t *pBot, edict_t *pEdict);
bool sameTeam (edict_t *e1, edict_t *e2);
int squadTeam (squad_t *pSquad);
squad_t *assignSquad (bot_t *pBot);
SQUAD_POSITION getSquadPosition (bot_t *pBot);
SQUAD_ROLE getSquadRole (bot_t *pBot);
bot_t *findRandomSquadMember (squad_t *pSquad);
bool BotSquad (bot_t *pBot);
float nearestSquadmate (bot_t *pBot);


#endif // AI_SQUAD_FUNC_H
