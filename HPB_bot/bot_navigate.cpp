//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_navigate.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"


extern int mod_id;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;  // number of waypoints currently in use


void BotFixIdealPitch(edict_t *pEdict)
{
   // check for wrap around of angle...
   if (pEdict->v.idealpitch > 180)
      pEdict->v.idealpitch -= 360;

   if (pEdict->v.idealpitch < -180)
      pEdict->v.idealpitch += 360;
}


float BotChangePitch( bot_t *pBot, float speed )
{
   edict_t *pEdict = pBot->pEdict;
   float ideal;
   float current;
   float current_180;  // current +/- 180 degrees
   float diff;

   // turn from the current v_angle pitch to the idealpitch by selecting
   // the quickest way to turn to face that direction
   
   current = pEdict->v.v_angle.x;

   ideal = pEdict->v.idealpitch;

   // find the difference in the current and ideal angle
   diff = abs(current - ideal);

   // check if the bot is already facing the idealpitch direction...
   if (diff <= 1)
      return diff;  // return number of degrees turned

   // check if difference is less than the max degrees per turn
   if (diff < speed)
      speed = diff;  // just need to turn a little bit (less than max)

   // here we have four cases, both angle positive, one positive and
   // the other negative, one negative and the other positive, or
   // both negative.  handle each case separately...

   if ((current >= 0) && (ideal >= 0))  // both positive
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }
   else if ((current >= 0) && (ideal < 0))
   {
      current_180 = current - 180;

      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else if ((current < 0) && (ideal >= 0))
   {
      current_180 = current + 180;
      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else  // (current < 0) && (ideal < 0)  both negative
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }

   // check for wrap around of angle...
   if (current > 180)
      current -= 360;
   if (current < -180)
      current += 360;

   pEdict->v.v_angle.x = current;

   return speed;  // return number of degrees turned
}


void BotFixIdealYaw(edict_t *pEdict)
{
   // check for wrap around of angle...
   if (pEdict->v.ideal_yaw > 180)
      pEdict->v.ideal_yaw -= 360;

   if (pEdict->v.ideal_yaw < -180)
      pEdict->v.ideal_yaw += 360;
}


float BotChangeYaw( bot_t *pBot, float speed )
{
   edict_t *pEdict = pBot->pEdict;
   float ideal;
   float current;
   float current_180;  // current +/- 180 degrees
   float diff;

   // turn from the current v_angle yaw to the ideal_yaw by selecting
   // the quickest way to turn to face that direction
   
   current = pEdict->v.v_angle.y;

   ideal = pEdict->v.ideal_yaw;

   // find the difference in the current and ideal angle
   diff = abs(current - ideal);

   // check if the bot is already facing the ideal_yaw direction...
   if (diff <= 1)
      return diff;  // return number of degrees turned

   // check if difference is less than the max degrees per turn
   if (diff < speed)
      speed = diff;  // just need to turn a little bit (less than max)

   // here we have four cases, both angle positive, one positive and
   // the other negative, one negative and the other positive, or
   // both negative.  handle each case separately...

   if ((current >= 0) && (ideal >= 0))  // both positive
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }
   else if ((current >= 0) && (ideal < 0))
   {
      current_180 = current - 180;

      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else if ((current < 0) && (ideal >= 0))
   {
      current_180 = current + 180;
      if (current_180 > ideal)
         current += speed;
      else
         current -= speed;
   }
   else  // (current < 0) && (ideal < 0)  both negative
   {
      if (current > ideal)
         current -= speed;
      else
         current += speed;
   }

   // check for wrap around of angle...
   if (current > 180)
      current -= 360;
   if (current < -180)
      current += 360;

   pEdict->v.v_angle.y = current;

   return speed;  // return number of degrees turned
}


bool BotFindWaypoint( bot_t *pBot, int team )
{
   // Do whatever you want here to select the next best waypoint.  Return
   // TRUE if another waypoint was found, or FALSE if another waypoint was
   // not found.

   return FALSE;  // couldn't find a waypoint
}


bool BotHeadTowardWaypoint( bot_t *pBot )
{
   // Do whatever you want here to get the bot to head towards the current
   // waypoint.  Remember to handle movement on land, underwater, on ladders,
   // and in the air (jetpacks) if necessary.  Return TRUE if the bot can
   // still get to the next waypoint, or FALSE if the next waypoint is now
   // unreachable for some reason.

   return FALSE;  // couldn't get to next waypoint
}

