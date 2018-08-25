//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"
#include "bot_weapons.h"

#include <sys/types.h>
#include <sys/stat.h>


extern int mod_id;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;  // number of waypoints currently in use

static FILE *fp;


#define PLAYER_SEARCH_RADIUS 40.0


bot_t bots[32];   // max of 32 bots in a game

int   need_init = 1;

#define VALVE_MAX_SKINS    10
#define GEARBOX_MAX_SKINS  20

// indicate which models are currently used for random model allocation
bool valve_skin_used[VALVE_MAX_SKINS] = {
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};

bool gearbox_skin_used[GEARBOX_MAX_SKINS] = {
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
   FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};

// store the names of the models...
char *valve_bot_skins[VALVE_MAX_SKINS] = {
   "barney", "gina", "gman", "gordon", "helmet",
   "hgrunt", "recon", "robo", "scientist", "zombie"};

char *gearbox_bot_skins[GEARBOX_MAX_SKINS] = {
   "barney", "beret", "cl_suit", "drill", "fassn", "gina", "gman",
   "gordon", "grunt", "helmet", "hgrunt", "massn", "otis", "recon",
   "recruit", "robo", "scientist", "shephard", "tower", "zombie"};

// store the player names for each of the models...
char *valve_bot_names[VALVE_MAX_SKINS] = {
   "Barney", "Gina", "G-Man", "Gordon", "Helmet",
   "H-Grunt", "Recon", "Robo", "Scientist", "Zombie"};

char *gearbox_bot_names[GEARBOX_MAX_SKINS] = {
   "Barney", "Beret", "Cl_suit", "Drill", "Fassn", "Gina", "G-Man",
   "Gordon", "Grunt", "Helmet", "H-Grunt", "Massn", "Otis", "Recon",
   "Recruit", "Robo", "Scientist", "Shepard", "Tower", "Zombie"};

// how often (out of 1000 times) the bot will pause, based on bot skill
float pause_frequency[5] = {4, 7, 10, 15, 20};

float pause_time[5][2] = {
   {0.2, 0.5}, {0.5, 1.0}, {0.7, 1.3}, {1.0, 1.7}, {1.2, 2.0}};


inline edict_t *CREATE_FAKE_CLIENT( const char *netname )
{
   return (*g_engfuncs.pfnCreateFakeClient)( netname );
}

inline char *GET_INFOBUFFER( edict_t *e )
{
   return (*g_engfuncs.pfnGetInfoKeyBuffer)( e );
}

inline char *GET_INFO_KEY_VALUE( char *infobuffer, char *key )
{
   return (g_engfuncs.pfnInfoKeyValue( infobuffer, key ));
}

inline void SET_CLIENT_KEY_VALUE( int clientIndex, char *infobuffer,
                                  char *key, char *value )
{
   (*g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, key, value );
}


extern "C"
{
   // this is the LINK_ENTITY_TO_CLASS function that creates a player (bot)
   void player(entvars_t *pev);
}


void BotSpawnInit( bot_t *pBot )
{
   pBot->v_prev_origin = Vector(9999.0, 9999.0, 9999.0);

   pBot->msecnum = 0;
   pBot->msecdel = 0.0;
   pBot->msecval = 0.0;

   pBot->bot_health = 0;
   pBot->bot_armor = 0;
   pBot->bot_money = 0;

   pBot->f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");

   pBot->prev_speed = 0.0;  // fake "paused" since bot is NOT stuck

   pBot->pBotEnemy = NULL;  // no enemy yet

   memset(&(pBot->current_weapon), 0, sizeof(pBot->current_weapon));
   memset(&(pBot->m_rgAmmo), 0, sizeof(pBot->m_rgAmmo));

   pBot->need_to_initialize = FALSE;
}


void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4)
{
   edict_t *BotEnt;
   bot_t *pBot;

   // initialize the bots array of structures if first time here...
   if (need_init == 1)
   {
      need_init = 0;
      memset(bots, 0, sizeof(bots));
   }

   BotEnt = CREATE_FAKE_CLIENT( "Bot" );

   if (FNullEnt( BotEnt ))
   {
      if (pPlayer)
         ClientPrint( VARS(pPlayer), HUD_PRINTNOTIFY, "Max. Players reached.  Can't create bot!\n");
   }
   else
   {
      char ptr[128];  // allocate space for message from ClientConnect
      char *infobuffer;
      int clientIndex;
      int index;

      if (IS_DEDICATED_SERVER())
         printf("Creating bot...\n");
      else if (pPlayer)
         ClientPrint( VARS(pPlayer), HUD_PRINTNOTIFY, "Creating bot...\n");

      index = 0;
      while ((bots[index].is_used) && (index < 32))
         index++;

      if (index == 32)
      {
         ClientPrint( VARS(pPlayer), HUD_PRINTNOTIFY, "Can't create bot!\n");
         return;
      }

      // create the player entity by calling MOD's player function
      // (from LINK_ENTITY_TO_CLASS for player object)

      player( VARS(BotEnt) );

      infobuffer = GET_INFOBUFFER( BotEnt );
      clientIndex = ENTINDEX( BotEnt );


      SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", "gina" );

      if (mod_id == CSTRIKE_DLL)
      {
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "rate", "3500.000000");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_updaterate", "20");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_lw", "1");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_lc", "1");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "tracker", "0");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "cl_dlmax", "128");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "lefthand", "1");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "friends", "0");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "dm", "0");
         SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "ah", "1");
      }

      ClientConnect( BotEnt, "Bot", "127.0.0.1", ptr );

      // Pieter van Dijk - use instead of DispatchSpawn() - Hip Hip Hurray!
      ClientPutInServer( BotEnt );

      BotEnt->v.flags |= FL_FAKECLIENT;

      // initialize all the variables for this bot...

      pBot = &bots[index];

      pBot->is_used = TRUE;
      pBot->respawn_state = RESPAWN_IDLE;
      pBot->name[0] = 0;  // name not set by server yet

      pBot->pEdict = BotEnt;

      pBot->not_started = 1;  // hasn't joined game yet

      if (mod_id == TFC_DLL)
         pBot->start_action = MSG_TFC_IDLE;
      else if (mod_id == CSTRIKE_DLL)
         pBot->start_action = MSG_CS_IDLE;
      else
         pBot->start_action = 0;  // not needed for non-team MODs


      BotSpawnInit(pBot);

      BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
      BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;
      BotEnt->v.pitch_speed = BOT_PITCH_SPEED;
      BotEnt->v.yaw_speed = BOT_YAW_SPEED;

      pBot->bot_team = -1;  // don't know what these are yet, server can change them
      pBot->bot_class = -1;

      if ((mod_id == TFC_DLL) || (mod_id == CSTRIKE_DLL))
      {
         if ((arg1 != NULL) && (arg1[0] != 0))
         {
            pBot->bot_team = atoi(arg1);

            if ((arg2 != NULL) && (arg2[0] != 0))
            {
               pBot->bot_class = atoi(arg2);
            }
         }
      }
   }
}


void BotStartGame( bot_t *pBot )
{
   char c_team[32];
   char c_class[32];

   edict_t *pEdict = pBot->pEdict;

   if (mod_id == TFC_DLL)
   {
      // handle Team Fortress Classic stuff here...

      if (pBot->start_action == MSG_TFC_TEAM_SELECT)
      {
         pBot->start_action = MSG_TFC_IDLE;  // switch back to idle

         if ((pBot->bot_team != 1) && (pBot->bot_team != 2) &&
             (pBot->bot_team != 5))
            pBot->bot_team = -1;

         if (pBot->bot_team == -1)
            pBot->bot_team = RANDOM_LONG(1, 2);

         // select the team the bot wishes to join...
         if (pBot->bot_team == 1)
            strcpy(c_team, "1");
         else if (pBot->bot_team == 2)
            strcpy(c_team, "2");
         else
            strcpy(c_team, "5");

         FakeClientCommand(pEdict, "jointeam", c_team, NULL);

         return;
      }

      if (pBot->start_action == MSG_TFC_CLASS_SELECT)
      {
         pBot->start_action = MSG_TFC_IDLE;  // switch back to idle

         if ((pBot->bot_class < 0) || (pBot->bot_class > 10))
            pBot->bot_class = -1;

         if (pBot->bot_class == -1)
            pBot->bot_class = RANDOM_LONG(1, 10);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 0)
            strcpy(c_class, "civilian");
         else if (pBot->bot_class == 1)
            strcpy(c_class, "scout");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "sniper");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "soldier");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "demoman");
         else if (pBot->bot_class == 5)
            strcpy(c_class, "medic");
         else if (pBot->bot_class == 6)
            strcpy(c_class, "hwguy");
         else if (pBot->bot_class == 7)
            strcpy(c_class, "pyro");
         else if (pBot->bot_class == 8)
            strcpy(c_class, "spy");
         else if (pBot->bot_class == 9)
            strcpy(c_class, "engineer");
         else
            strcpy(c_class, "randompc");

         FakeClientCommand(pEdict, c_class, NULL, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;

         return;
      }
   }
   else if (mod_id == CSTRIKE_DLL)
   {
      // handle Counter-Strike stuff here...

      if (pBot->start_action == MSG_CS_TEAM_SELECT)
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_team != 1) && (pBot->bot_team != 2) &&
             (pBot->bot_team != 5))
            pBot->bot_team = -1;

         if (pBot->bot_team == -1)
            pBot->bot_team = RANDOM_LONG(1, 2);

         // select the team the bot wishes to join...
         if (pBot->bot_team == 1)
            strcpy(c_team, "1");
         else if (pBot->bot_team == 2)
            strcpy(c_team, "2");
         else
            strcpy(c_team, "5");

         FakeClientCommand(pEdict, "menuselect", c_team, NULL);

         return;
      }

      if (pBot->start_action == MSG_CS_CT_SELECT)  // counter terrorist
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_class < 1) || (pBot->bot_class > 4))
            pBot->bot_class = -1;  // use random if invalid

         if (pBot->bot_class == -1)
            pBot->bot_class = RANDOM_LONG(1, 4);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 1)
            strcpy(c_class, "1");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "2");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "3");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "4");
         else
            strcpy(c_class, "5");  // random

         FakeClientCommand(pEdict, "menuselect", c_class, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;

         return;
      }

      if (pBot->start_action == MSG_CS_T_SELECT)  // terrorist select
      {
         pBot->start_action = MSG_CS_IDLE;  // switch back to idle

         if ((pBot->bot_class < 1) || (pBot->bot_class > 4))
            pBot->bot_class = -1;  // use random if invalid

         if (pBot->bot_class == -1)
            pBot->bot_class = RANDOM_LONG(1, 4);

         // select the class the bot wishes to use...
         if (pBot->bot_class == 1)
            strcpy(c_class, "1");
         else if (pBot->bot_class == 2)
            strcpy(c_class, "2");
         else if (pBot->bot_class == 3)
            strcpy(c_class, "3");
         else if (pBot->bot_class == 4)
            strcpy(c_class, "4");
         else
            strcpy(c_class, "5");  // random

         FakeClientCommand(pEdict, "menuselect", c_class, NULL);

         // bot has now joined the game (doesn't need to be started)
         pBot->not_started = 0;

         return;
      }
   }
   else
   {
      // otherwise, don't need to do anything to start game...
      pBot->not_started = 0;
   }
}


int BotInFieldOfView(bot_t *pBot, Vector dest)
{
   // find angles from source to destination...
   Vector entity_angles = UTIL_VecToAngles( dest );

   // make yaw angle 0 to 360 degrees if negative...
   if (entity_angles.y < 0)
      entity_angles.y += 360;

   // get bot's current view angle...
   float view_angle = pBot->pEdict->v.v_angle.y;

   // make view angle 0 to 360 degrees if negative...
   if (view_angle < 0)
      view_angle += 360;

   // return the absolute value of angle to destination entity
   // zero degrees means straight ahead,  45 degrees to the left or
   // 45 degrees to the right is the limit of the normal view angle

   // rsm - START angle bug fix
   int angle = abs((int)view_angle - (int)entity_angles.y);

   if (angle > 180)
      angle = 360 - angle;

   return angle;
   // rsm - END
}


bool BotEntityIsVisible( bot_t *pBot, Vector dest )
{
   TraceResult tr;

   // trace a line from bot's eyes to destination...
   UTIL_TraceLine( pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
                   dest, ignore_monsters,
                   pBot->pEdict->v.pContainingEntity, &tr );

   // check if line of sight to object is not blocked (i.e. visible)
   if (tr.flFraction >= 1.0)
      return TRUE;
   else
      return FALSE;
}


void BotThink( bot_t *pBot )
{
   int index = 0;
   Vector v_diff;             // vector from previous to current location
   float yaw_degrees;
   float moved_distance;      // length of v_diff vector (distance bot moved)
   TraceResult tr;
   bool found_waypoint;

   edict_t *pEdict = pBot->pEdict;


   pEdict->v.flags |= FL_FAKECLIENT;

   if (pBot->name[0] == 0)  // name filled in yet?
      strcpy(pBot->name, STRING(pBot->pEdict->v.netname));


// TheFatal - START from Advanced Bot Framework (Thanks Rich!)

   // adjust the millisecond delay based on the frame rate interval...
   if (pBot->msecdel <= gpGlobals->time)
   {
      pBot->msecdel = gpGlobals->time + 0.5;
      if (pBot->msecnum > 0)
         pBot->msecval = 450.0/pBot->msecnum;
      pBot->msecnum = 0;
   }
   else
      pBot->msecnum++;

   if (pBot->msecval < 5)    // don't allow msec to be less than 5...
      pBot->msecval = 5;

   if (pBot->msecval > 100)  // ...or greater than 100
      pBot->msecval = 100;

// TheFatal - END


   pEdict->v.button = 0;
   pBot->f_move_speed = 0.0;

   // if the bot hasn't selected stuff to start the game yet, go do that...
   if (pBot->not_started)
   {
      BotStartGame( pBot );

      g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, 0.0,
                                   0, 0, pEdict->v.button, 0, pBot->msecval);

      return;
   }

   // if the bot is dead, randomly press fire to respawn...
   if ((pEdict->v.health < 1) || (pEdict->v.deadflag != DEAD_NO))
   {
      if (pBot->need_to_initialize)
         BotSpawnInit(pBot);

      if (RANDOM_LONG(1, 100) > 50)
         pEdict->v.button = IN_ATTACK;

      g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, pBot->f_move_speed,
                                   0, 0, pEdict->v.button, 0, pBot->msecval);

      return;
   }

   // set this for the next time the bot dies so it will initialize stuff
   pBot->need_to_initialize = TRUE;

   pBot->f_move_speed = pBot->f_max_speed;  // set to max speed


   // see how far bot has moved since the previous position...
   v_diff = pBot->v_prev_origin - pEdict->v.origin;
   moved_distance = v_diff.Length();

   // save current position as previous
   pBot->v_prev_origin = pEdict->v.origin;

   // turn towards ideal_yaw by yaw_speed degrees
   yaw_degrees = BotChangeYaw( pBot, pEdict->v.yaw_speed );

   if (yaw_degrees >= pEdict->v.yaw_speed)
   {
      pBot->f_move_speed = 0.0;  // don't move while turning a lot
   }
   else if (yaw_degrees >= 10)   // turning more than 10 degrees?
   {
      pBot->f_move_speed = pBot->f_move_speed / 4;  // slow down while turning
   }
   else  // else handle movement related actions...
   {
      // see if bot can find an enemy...

      pBot->pBotEnemy = BotFindEnemy( pBot );

      if (pBot->pBotEnemy != NULL)  // does an enemy exist?
      {
         BotShootAtEnemy( pBot );  // shoot at the enemy
      }
      else
      {
         // no enemy, let's just wander around...

         pEdict->v.v_angle.z = 0;  // reset roll to 0 (straight up and down)

         // set body angles to match view angles...
         pEdict->v.angles.x = 0;
         pEdict->v.angles.y = pEdict->v.v_angle.y;
         pEdict->v.angles.z = 0;


         found_waypoint = BotHeadTowardWaypoint(pBot);

         // if the bot isn't headed toward a waypoint...
         if (found_waypoint == FALSE)
         {
            // check if the bot hasn't moved much since the last location
            if ((moved_distance <= 1.0) && (pBot->prev_speed >= 1.0))
            {
               // the bot must be stuck!
               pBot->f_move_speed = 0;  // don't move while turning
            
               if (RANDOM_LONG(1, 100) <= 10)
               {
                  // 10 percent of the time turn completely around...
                  pBot->pEdict->v.ideal_yaw += 180;
               }
               else
               {
                  // turn randomly between 30 and 60 degress
                  if (RANDOM_LONG(1, 100) <= 50)
                     pBot->pEdict->v.ideal_yaw += RANDOM_LONG(30, 60);
                  else
                     pBot->pEdict->v.ideal_yaw -= RANDOM_LONG(30, 60);
               }
            
               BotFixIdealYaw(pBot->pEdict);
            }
         }

         // should the bot pause for a while here?
         if (RANDOM_LONG(1, 100) <= 5)
         {
            // set the time that the bot will stop "pausing"
            pBot->f_pause_time = gpGlobals->time + 1.0;
         }
      }
   }

   if (pBot->f_pause_time > gpGlobals->time)  // is the bot "paused"?
      pBot->f_move_speed = 0;  // don't move while pausing

   // make the body face the same way the bot is looking
   pEdict->v.angles.y = pEdict->v.v_angle.y;

   // save the previous speed (for checking if stuck)
   pBot->prev_speed = pBot->f_move_speed;

   g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, pBot->f_move_speed,
                                0, 0, pEdict->v.button, 0, pBot->msecval);

   return;
}

