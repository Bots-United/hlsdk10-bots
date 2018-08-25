//
// HPB_bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_func.h
//

#ifndef BOT_FUNC_H
#define BOT_FUNC_H


//prototypes of bot functions...

void BotSpawnInit( bot_t *pBot );
void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4 );
void BotStartGame( bot_t *pBot );
int BotInFieldOfView( bot_t *pBot, Vector dest );
bool BotEntityIsVisible( bot_t *pBot, Vector dest );
void BotFindItem( bot_t *pBot );
void BotThink( bot_t *pBot );


void BotFixIdealPitch( edict_t *pEdict );
float BotChangePitch( bot_t *pBot, float speed );
void BotFixIdealYaw( edict_t *pEdict );
float BotChangeYaw( bot_t *pBot, float speed );
bool BotFindWaypoint( bot_t *pBot );
bool BotHeadTowardWaypoint( bot_t *pBot );

edict_t *BotFindEnemy( bot_t *pBot );
Vector BotBodyTarget( edict_t *pBotEnemy, bot_t *pBot );
bool BotFireWeapon( Vector v_enemy, bot_t *pBot );
void BotShootAtEnemy( bot_t *pBot );


#endif // BOT_FUNC_H

