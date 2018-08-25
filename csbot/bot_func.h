/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************
Adapted from:
   HPB_bot
   botman's High Ping Bastard bot
   http://planethalflife.com/botman/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/bot_func.h,v $
$Revision: 1.4 $
$Date: 2001/04/30 08:06:11 $
$State: Exp $

***********************************

$Log: bot_func.h,v $
Revision 1.4  2001/04/30 08:06:11  eric
willFire() deals with crouching versus standing

Revision 1.3  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef BOT_FUNC_H
#define BOT_FUNC_H


// bot.cpp
void BotSpawnInit (bot_t *pBot);
void BotCreate (edict_t *pPlayer, const char *arg1, const char *arg2, const char *arg3, const char *arg4);
void BotStartGame (bot_t *pBot);
void BotThink (bot_t *pBot);

// bot_navigate.cpp
bool traceHit (Vector src, Vector dest, edict_t *containing, bool ignore);
bool traceHit (Vector src, Vector dest, edict_t *containing);
void resetAllSquadsNav (void);
int currentDestination (squad_t *pSquad);
void setCurrentWaypoint (bot_t *pBot, int wp);
bool BotHeadTowardWaypoint (bot_t *pBot);
bool obstacleJump (bot_t *pBot);
bool obstacleDuck (bot_t *pBot);
bool obstacleFront (bot_t *pBot);
bool obstacleLeft (bot_t *pBot);
bool obstacleRight (bot_t *pBot);


#endif // BOT_FUNC_H

