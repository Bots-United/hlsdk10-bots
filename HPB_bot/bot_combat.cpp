//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_combat.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"

extern int mod_id;
extern bot_weapon_t weapon_defs[MAX_WEAPONS];


edict_t *BotFindEnemy( bot_t *pBot )
{
   int i;
   float nearestdistance = 1000;  // max distance of enemy
   static float is_team_play;
   Vector vecEnd;
   edict_t *pNewEnemy = NULL;

   edict_t *pEdict = pBot->pEdict;

   // search the world for players...
   for (i = 1; i <= gpGlobals->maxClients; i++)
   {
      edict_t *pPlayer = INDEXENT(i);

      // skip invalid players and skip self (i.e. this bot)
      if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict))
      {
         // skip this player if not alive (i.e. dead or dying)
         if (!IsAlive(pPlayer))
            continue;

         // is this TFC or Counter-Strike?
         if ((mod_id == TFC_DLL) || (mod_id == CSTRIKE_DLL))
            is_team_play = 1.0;
         else
            is_team_play = CVAR_GET_FLOAT("mp_teamplay");  // teamplay enabled?

         // is team play enabled?
         if (is_team_play > 0.0)
         {
            int player_team = UTIL_GetTeam(pPlayer);
            int bot_team = UTIL_GetTeam(pEdict);

            // don't target your teammates...
            if (bot_team == player_team)
               continue;
         }

         vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

         // see if bot can see the player...
         if (FInViewCone( &vecEnd, pEdict ) &&
             FVisible( vecEnd, pEdict ))
         {
            float distance;
            distance = (pPlayer->v.origin - pEdict->v.origin).Length();
            if (distance < nearestdistance)
            {
               nearestdistance = distance;
               pNewEnemy = pPlayer;
            }
         }
      }
   }

   if (pNewEnemy)
   {
      // face the enemy
      Vector v_enemy = pNewEnemy->v.origin - pEdict->v.origin;
      Vector bot_angles = UTIL_VecToAngles( v_enemy );

      pEdict->v.ideal_yaw = bot_angles.y;

      BotFixIdealYaw(pEdict);
   }

   return (pNewEnemy);
}


Vector BotBodyTarget( edict_t *pBotEnemy, bot_t *pBot )
{
   Vector target;
   float f_distance;
   float f_scale;
   int d_x, d_y, d_z;

   edict_t *pEdict = pBot->pEdict;

   f_distance = (pBotEnemy->v.origin - pEdict->v.origin).Length();

   if (f_distance > 1000)
      f_scale = 1.0;
   else if (f_distance > 100)
      f_scale = f_distance / 1000.0;
   else
      f_scale = 0.1;

   // randomly select the aiming style...
   switch (RANDOM_LONG(0,4))
   {
      case 0:
         // VERY GOOD, same as from CBasePlayer::BodyTarget (in player.h)
         target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs * RANDOM_FLOAT( 0.5, 1.1 );
         d_x = 0;  // no offset
         d_y = 0;
         d_z = 0;
         break;
      case 1:
         // GOOD, offset a little for x, y, and z
         target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;  // aim for the head (if you can find it)
         d_x = RANDOM_FLOAT(-5, 5) * f_scale;
         d_y = RANDOM_FLOAT(-5, 5) * f_scale;
         d_z = RANDOM_FLOAT(-10, 10) * f_scale;
         break;
      case 2:
         // FAIR, offset somewhat for x, y, and z
         target = pBotEnemy->v.origin;  // aim for the body
         d_x = RANDOM_FLOAT(-10, 10) * f_scale;
         d_y = RANDOM_FLOAT(-10, 10) * f_scale;
         d_z = RANDOM_FLOAT(-18, 18) * f_scale;
         break;
      case 3:
         // POOR, offset for x, y, and z
         target = pBotEnemy->v.origin;  // aim for the body
         d_x = RANDOM_FLOAT(-20, 20) * f_scale;
         d_y = RANDOM_FLOAT(-20, 20) * f_scale;
         d_z = RANDOM_FLOAT(-32, 32) * f_scale;
         break;
      case 4:
         // BAD, offset lots for x, y, and z
         target = pBotEnemy->v.origin;  // aim for the body
         d_x = RANDOM_FLOAT(-35, 35) * f_scale;
         d_y = RANDOM_FLOAT(-35, 35) * f_scale;
         d_z = RANDOM_FLOAT(-50, 50) * f_scale;
         break;
   }

   target = target + Vector(d_x, d_y, d_z);

   return target;
}


bool BotFireWeapon( Vector v_enemy, bot_t *pBot )
{
   // Use the Vector argument of the emeny to aim at the enemy and fire
   // here.  Return TRUE if bot fired weapon (enough ammo, etc.) otherwise
   // return FALSE to indicate failure (maybe bot would runaway then).

   pBot->pEdict->v.button |= IN_ATTACK;

   return TRUE;
}


void BotShootAtEnemy( bot_t *pBot )
{
   edict_t *pEdict = pBot->pEdict;

   // aim for the head and/or body
   Vector v_enemy = BotBodyTarget( pBot->pBotEnemy, pBot ) - GetGunPosition(pEdict);

   pEdict->v.v_angle = UTIL_VecToAngles( v_enemy );

   // Paulo-La-Frite - START bot aiming bug fix
   if (pEdict->v.v_angle.x > 180)
      pEdict->v.v_angle.x -=360;

   // set the body angles to point the gun correctly
   pEdict->v.angles.x = pEdict->v.v_angle.x / 3;
   pEdict->v.angles.y = pEdict->v.v_angle.y;
   pEdict->v.angles.z = 0;

   // adjust the view angle pitch to aim correctly (MUST be after body v.angles stuff)
   pEdict->v.v_angle.x = -pEdict->v.v_angle.x;
   // Paulo-La-Frite - END

   pEdict->v.ideal_yaw = pEdict->v.v_angle.y;

   BotFixIdealYaw(pEdict);

   // select the best weapon to use at this distance and fire...
   BotFireWeapon( v_enemy, pBot );
}

