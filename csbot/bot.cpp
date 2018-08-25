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

$Source: /usr/local/cvsroot/csbot/src/dlls/bot.cpp,v $
$Revision: 1.16 $
$Date: 2001/05/11 07:06:19 $
$State: Exp $

***********************************

$Log: bot.cpp,v $
Revision 1.16  2001/05/11 07:06:19  eric
bots respond to being stuck

Revision 1.15  2001/05/11 06:17:23  eric
navigation cleanup

Revision 1.14  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.13  2001/05/09 05:03:39  eric
bugfixes from botman

Revision 1.12  2001/05/06 12:53:39  eric
Prepping windows build for initial release

Revision 1.11  2001/05/06 01:48:37  eric
Replaced global constants with enums where appropriate

Revision 1.10  2001/05/03 19:35:16  eric
CGF-style aiming

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

Revision 1.4  2001/04/29 04:13:41  eric
Trimmed down bot structure

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
#include "bot_func.h"
#include "waypoint.h"
#include "bot_weapons.h"

#include <sys/types.h>
#include <sys/stat.h>


#ifndef __linux__
extern HINSTANCE h_Library;
#else
extern void *h_Library;
#endif


extern float respawn_time;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern int num_waypoints;  // number of waypoints currently in use
extern int default_bot_skill;

extern char bot_whine[MAX_BOT_WHINE][81];
extern int whine_count;
extern bool bomb_planted;
extern bool bomb_on_ground;

static FILE *fp;


#define PLAYER_SEARCH_RADIUS     40.0


bot_t bots[MAX_BOTS];
bool b_observer_mode = false;
bool b_botdontshoot = false;
float round_start_time = 0.0;

extern squad_t squads[MAX_SQUADS];
extern int recent_bot_whine[5];

int number_names = 0;

#define MAX_BOT_NAMES 100

char bot_names[MAX_BOT_NAMES][BOT_NAME_LEN+1];

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


// this is the LINK_ENTITY_TO_CLASS function that creates a player (bot)
void player( entvars_t *pev )
{
   static LINK_ENTITY_FUNC otherClassName = NULL;
   if (otherClassName == NULL)
      otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(h_Library, "player");
   if (otherClassName != NULL)
   {
      (*otherClassName)(pev);
   }
}


/** Called at the beginning of every round. */
void round_begin (void) {
	round_start_time = gpGlobals->time;
	bomb_planted = false;
	bomb_on_ground = false;

	for (int i = 0; i < MAX_BOTS; i++) {
		if (bots[i].is_used)
			bots[i].pSquad = &squads[i]; // assigns bots to their own squad
	}

	for (int j = 0; j < MAX_BOTS; j++) {
		if (bots[j].is_used)
			BotSpawnInit (&bots[j]); // TODO: this may be wrong (e.g. it may take away guns even if they didn't die)
	}

	resetAllSquadsNav ();
}


void BotSpawnInit( bot_t *pBot )
{
   int i;

   pBot->movement_time = gpGlobals->time;
   pBot->defuse_bomb_time = 0.0;
   pBot->plant_bomb_time = 0.0;

   pBot->start_action = MSG_CS_BUYING;

   for (i = 0; i < VIEW_HISTORY_SIZE; i++)
      pBot->prev_v_angle[i] = Vector (0,0,0);

   for (i = 0; i < VEL_HISTORY_SIZE; i++)
      pBot->prev_velocity[i] = Vector (0,0,0);

   pBot->wp_origin = Vector (0, 0, 0);
   pBot->wp_last = WP_INVALID;
   pBot->wp_current = WP_INVALID;
   pBot->wp_tactical = WP_INVALID;
   pBot->tactical_time = gpGlobals->time;

   pBot->msecnum = 0;
   pBot->msecdel = 0.0;
   pBot->msecval = 0.0;

   pBot->blinded_time = 0.0;

   pBot->buy_num = NULL;
   pBot->buy_cat = NULL;
   pBot->buy_time = RANDOM_FLOAT (1, 5);
   pBot->buy_armor_count = 0;
   pBot->buy_ammo_count = 0;
   pBot->buy_equip_count = 0;

   pBot->radio_type = NULL;
   pBot->radio_num = NULL;
   pBot->radio_time = 0.0;
   pBot->speak_time = 0.0;
   pBot->radio_time_down = 0.0;

   pBot->max_speed = CVAR_GET_FLOAT("sv_maxspeed");
   pBot->move_speed = 0.0;
   pBot->prev_origin = Vector (0, 0, 0);
   pBot->prev_speed = 0.0;

   pBot->ladder_dir = LADDER_UNKNOWN;
   pBot->wp_ladder_top = false;

   pBot->desiredVelocity = Vector(0, 0, 0);
   pBot->lookAt = Vector(1, 1, 0); // point on unit circle

   pBot->threat = NULL;
   pBot->last_seen_threat_location = Vector (0, 0, 0);
   pBot->last_seen_threat_time = gpGlobals->time;
   pBot->threat_giveup_time = RANDOM_FLOAT (2.0, 4.0);

   pBot->killer_edict = NULL;
   pBot->b_bot_say_killed = false;
   pBot->f_bot_say_killed = 0.0;

   pBot->shoot_time = gpGlobals->time;
   pBot->shoot_stop_time = gpGlobals->time;
   pBot->aim_head = Vector (0,0,0);
   pBot->aim_deviation = Vector (0,0,0);
   pBot->aim_deviation_scale = 0.0;

   pBot->movethink_time = gpGlobals->time;
   pBot->movement_style = ADVANCE;
   pBot->squad_time = gpGlobals->time;
   pBot->pSquad = assignSquad (pBot);

   memset(&(pBot->current_weapon), 0, sizeof(pBot->current_weapon));
   memset(&(pBot->m_rgAmmo), 0, sizeof(pBot->m_rgAmmo));
}


void BotNameInit( void )
{
   FILE *bot_name_fp;
   char bot_name_filename[256];
   int str_index;
   char name_buffer[80];
   int length, index;

   UTIL_BuildFileName(bot_name_filename, "bot_names.txt", NULL);

   bot_name_fp = fopen(bot_name_filename, "r");

   if (bot_name_fp != NULL)
   {
      while ((number_names < MAX_BOT_NAMES) &&
             (fgets(name_buffer, 80, bot_name_fp) != NULL))
      {
         length = strlen(name_buffer);

         if (name_buffer[length-1] == '\n')
         {
            name_buffer[length-1] = 0;  // remove '\n'
            length--;
         }

         str_index = 0;
         while (str_index < length)
         {
            if ((name_buffer[str_index] < ' ') || (name_buffer[str_index] > '~') ||
                (name_buffer[str_index] == '"'))
            for (index=str_index; index < length; index++)
               name_buffer[index] = name_buffer[index+1];

            str_index++;
         }

         if (name_buffer[0] != 0)
         {
            strncpy(bot_names[number_names], name_buffer, BOT_NAME_LEN);

            number_names++;
         }
      }

      fclose(bot_name_fp);
   }
}


void BotPickName( char *name_buffer )
{
   int name_index, index;
   bool used;
   edict_t *pPlayer;
   int attempts = 0;

   // see if a name exists from a kicked bot (if so, reuse it)
   for (index=0; index < MAX_BOTS; index++)
   {
      if ((bots[index].is_used == FALSE) && (bots[index].name[0]))
      {
         strcpy(name_buffer, bots[index].name);

         return;
      }   
   }

   name_index = RANDOM_LONG(1, number_names) - 1;  // zero based

   // check make sure this name isn't used
   used = TRUE;

   while (used)
   {
      used = FALSE;

      for (index = 1; index <= gpGlobals->maxClients; index++)
      {
         pPlayer = INDEXENT(index);

         if (pPlayer && !pPlayer->free)
         {
            if (strcmp(bot_names[name_index], STRING(pPlayer->v.netname)) == 0)
            {
               used = TRUE;
               break;
            }
         }
      }

      if (used)
      {
         name_index++;

         if (name_index == number_names)
            name_index = 0;

         attempts++;

         if (attempts == number_names)
            used = FALSE;  // break out of loop even if already used
      }
   }

   strcpy(name_buffer, bot_names[name_index]);
}


void BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4)
{
   edict_t *BotEnt;
   bot_t *pBot;
   char c_skin[BOT_SKIN_LEN+1];
   char c_name[BOT_NAME_LEN+1];
   int skill;
   int i, j, length;
   bool found = FALSE;

   if ((arg3 != NULL) && (*arg3 != 0))
   {
      strncpy( c_name, arg3, BOT_NAME_LEN-1 );
      c_name[BOT_NAME_LEN] = 0;  // make sure c_name is null terminated
   }
   else
   {
      if (number_names > 0)
         BotPickName( c_name );
      else
         strcpy(c_name, "Bot");
   }

   skill = 0;

   if ((arg4 != NULL) && (*arg4 != 0))
      skill = atoi(arg4);
         
   if ((skill < 1) || (skill > 100))
      skill = default_bot_skill;

   length = strlen(c_name);

   // remove any illegal characters from name...
   for (i = 0; i < length; i++)
   {
      if ((c_name[i] <= ' ') || (c_name[i] > '~') ||
          (c_name[i] == '"'))
      {
         for (j = i; j < length; j++)  // shuffle chars left (and null)
            c_name[j] = c_name[j+1];
         length--;
      }               
   }

   BotEnt = CREATE_FAKE_CLIENT( c_name );

   if (FNullEnt( BotEnt ))
   {
      if (pPlayer)
         ClientPrint( pPlayer, HUD_PRINTNOTIFY, "Max. Players reached.  Can't create bot!\n");
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
         ClientPrint( pPlayer, HUD_PRINTNOTIFY, "Creating bot...\n");

      index = 0;
      while ((bots[index].is_used) && (index < MAX_BOTS))
         index++;

      if (index == MAX_BOTS)
      {
         ClientPrint( pPlayer, HUD_PRINTNOTIFY, "Can't create bot!\n");
         return;
      }

      // create the player entity by calling MOD's player function
      // (from LINK_ENTITY_TO_CLASS for player object)

      player( VARS(BotEnt) );

      infobuffer = GET_INFOBUFFER( BotEnt );
      clientIndex = ENTINDEX( BotEnt );

      SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", "gina" );

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

      ClientConnect( BotEnt, c_name, "127.0.0.1", ptr );

      // Pieter van Dijk - use instead of DispatchSpawn() - Hip Hip Hurray!
      ClientPutInServer( BotEnt );

      BotEnt->v.flags |= FL_FAKECLIENT;

      // initialize all the variables for this bot...

      pBot = &bots[index];

	  // initialize all squads with same index as created bots
	  SquadInit (&squads[index]);
	  pBot->pSquad = &squads[index];

      pBot->is_used = TRUE;
      pBot->respawn_state = RESPAWN_IDLE;
      pBot->create_time = gpGlobals->time;
      pBot->name[0] = 0;  // name not set by server yet
      pBot->bot_money = 0;

      strcpy(pBot->skin, c_skin);

      pBot->pEdict = BotEnt;

      pBot->not_started = 1;  // hasn't joined game yet

      pBot->start_action = MSG_CS_IDLE;

      BotSpawnInit(pBot);

	  pBot->need_to_initialize = false;

	  float deviance = RANDOM_FLOAT (-10, 10) + RANDOM_FLOAT (-10, 10) + RANDOM_FLOAT (-10, 10);
      pBot->bot_skill = clamp (skill + deviance, 1, 100);

      pBot->bot_team = -1;
      pBot->bot_class = -1;

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

void BotStartGame( bot_t *pBot )
{
	// TODO: have bots' teams be sticky
   //char c_team[32];
   char c_class[32];

   edict_t *pEdict = pBot->pEdict;

   pBot->bot_team = UTIL_GetTeam (pBot->pEdict);

   switch (pBot->start_action) {
   case MSG_CS_TEAM_SELECT: 
         pBot->start_action = MSG_CS_IDLE;
/*
         if (pBot->bot_team != 0 && pBot->bot_team != 1)
            pBot->bot_team = RANDOM_LONG(0, 4);

         if (pBot->bot_team == TERRORIST)
            strcpy(c_team, "1");
         else if (pBot->bot_team == COUNTER)
            strcpy(c_team, "2");
         else
            strcpy(c_team, "5");  // random

         FakeClientCommand(pEdict, "menuselect", c_team, NULL);
*/
		 FakeClientCommand(pEdict, "menuselect", "5", NULL);
		 break;
	case MSG_CS_CT_SELECT:
	case MSG_CS_T_SELECT:
         pBot->start_action = MSG_CS_IDLE;

         if (pBot->bot_class < 1 || pBot->bot_class > 4)
            pBot->bot_class = RANDOM_LONG(1, 4);  

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

         pBot->not_started = 0;
		 break;
	}
}

void BotThink( bot_t *pBot )
{
	int i;
	TraceResult tr;

	edict_t *pEdict = pBot->pEdict;
	pEdict->v.flags |= FL_FAKECLIENT;
	pEdict->v.button = 0;

	/* name */

	if (pBot->name[0] == 0)
		strcpy (pBot->name, STRING (pEdict->v.netname));

	/* dysnchronous time (courtesy TheFatal) */

	// adjust the millisecond delay based on the frame rate interval
	if (pBot->msecdel <= gpGlobals->time) {
		pBot->msecdel = gpGlobals->time + 0.5;
		if (pBot->msecnum > 0)
			pBot->msecval = 450.0 / pBot->msecnum;
		pBot->msecnum = 0;
	} else {
		pBot->msecnum++;
	}

	if (pBot->msecval < 1)
		pBot->msecval = 1;

	if (pBot->msecval > 100)
		pBot->msecval = 100;

	/* just starting */

	if (pBot->not_started) {
		BotStartGame (pBot);
		g_engfuncs.pfnRunPlayerMove (pEdict, pEdict->v.v_angle, 0, 0, 0, pEdict->v.button, 0, pBot->msecval);
		return;
	}

	/* whine */

	if (pBot->b_bot_say_killed && pBot->f_bot_say_killed < gpGlobals->time) {
		int whine_index;
		bool used;
		int i, recent_count;
		char msg[120];

		pBot->b_bot_say_killed = FALSE;
            
		recent_count = 0;

		while (recent_count < 5) {
			whine_index = RANDOM_LONG (0, whine_count - 1);
			used = FALSE;

			for (i = 0; i < 5; i++) {
				if (recent_bot_whine[i] == whine_index)
					used = TRUE;
			}

			if (used)
				recent_count++;
			else
				break;
		}

		for (i = 4; i > 0; i--)
			recent_bot_whine[i] = recent_bot_whine[i - 1];

		recent_bot_whine[0] = whine_index;

		if (pBot->killer_edict != NULL && strstr (bot_whine[whine_index], "%s") != NULL)  // is "%s" in whine text?
			sprintf (msg, bot_whine[whine_index], STRING (pBot->killer_edict->v.netname));
		else
			sprintf (msg, bot_whine[whine_index]);

		speak (pBot, msg);
	}

	/* death */

	if (! IsAlive (pEdict)) {
		if (pBot->need_to_initialize) {
			BotSpawnInit (pBot);

			// botman's fix. thank you!
			if (pBot->killer_edict != NULL && whine_count > 0 && (pBot->spawn_time + 15.0) <= gpGlobals->time) {
				if ((RANDOM_LONG(1,100) <= 10)) {
					pBot->b_bot_say_killed = true;
					pBot->f_bot_say_killed = gpGlobals->time + 10.0 + RANDOM_FLOAT(0.0, 5.0);
				}
			}

			pBot->need_to_initialize = false;
		}

		// randomly press fire while dead
		if (RANDOM_LONG (1, 100) > 50)
			pEdict->v.button = IN_ATTACK;

		g_engfuncs.pfnRunPlayerMove (pEdict, pEdict->v.v_angle, 0, 0, 0, pEdict->v.button, 0, pBot->msecval);
		return;
	}

	/* initialization */

	if (pBot->need_to_initialize == false) {
		pBot->need_to_initialize = true;
		pBot->spawn_time = gpGlobals->time;
		pBot->start_action = MSG_CS_BUYING;
	}

	/* squad (done before buying, so bot can get right weapon for the job) */

	BotSquad (pBot);

	/* buy */

	if (pBot->start_action == MSG_CS_BUYING
		&& (pBot->spawn_time + pBot->buy_time <= gpGlobals->time)
		&& (round_start_time + pBot->buy_time <= gpGlobals->time))
	{
		pBot->start_action = MSG_CS_MENU_WAITING; 
		FakeClientCommand (pEdict, "buy", NULL, NULL);
		g_engfuncs.pfnRunPlayerMove (pEdict, pEdict->v.v_angle, 0, 0, 0, pEdict->v.button, 0, pBot->msecval);
		return;
	}

	if (isBuyMsg (pBot->start_action)) {
		BotBuy (pBot);
		g_engfuncs.pfnRunPlayerMove (pEdict, pEdict->v.v_angle, 0, 0, 0, pEdict->v.button, 0, pBot->msecval);
		return;
	}

	/* radio */

	BotRadio (pBot);

	/* movement history */

	// TODO: smooth speed too

	pBot->move_speed = pBot->max_speed;

	if (pBot->movement_time <= gpGlobals->time) {
		pBot->movement_time = gpGlobals->time + HISTORY_SAMPLING_RATE;

		for (i = 0; i < VIEW_HISTORY_SIZE - 1; i++)
			pBot->prev_v_angle[i] = pBot->prev_v_angle[i + 1];
		pBot->prev_v_angle[VIEW_HISTORY_SIZE - 1] = pEdict->v.v_angle;

		for (i = 0; i < VEL_HISTORY_SIZE - 1; i++)
			pBot->prev_velocity[i] = pBot->prev_velocity[i + 1];
		pBot->prev_velocity[VIEW_HISTORY_SIZE - 1] = pBot->desiredVelocity;
	}

	/* blindedness */

	if (pBot->blinded_time > gpGlobals->time) {
		// TODO: do something cooler (move to sensory)
		pEdict->v.button |= IN_DUCK;
		g_engfuncs.pfnRunPlayerMove (pEdict, pEdict->v.v_angle, 0, 0, 0, pEdict->v.button, 0, pBot->msecval);
		return;
	}

	/* sensory */

	scan_environment (pBot);

	/* behavior */

	int will_fire = willFire (pBot);
	switch (pBot->movement_style) {
		case RAPID:
			if (will_fire != 0) {
				pBot->move_speed = pBot->max_speed / 2.0;
			} else {
				pBot->move_speed = pBot->max_speed;
			}
			break;
		case DEFEND:// TODO
		case STEALTH:
			if (will_fire != 0) {
				pBot->move_speed = 0.0;
				if (will_fire & CROUCH_CLEAR)
					pEdict->v.button |= IN_DUCK;
			} else {
				pBot->move_speed = pBot->max_speed / 2.0;
			}
			break;
		default:
		case COVER:// TODO
		case RETREAT:// TODO
		case FLANK:// TODO
		case ADVANCE:
			if (will_fire != 0) {
				pBot->move_speed = 0.0;
				if (will_fire & CROUCH_CLEAR)
					pEdict->v.button |= IN_DUCK;
			} else {
				pBot->move_speed = pBot->max_speed;
			}
			break;
	}

	/* get cover */

	if (pBot->threat != NULL && will_fire == 0)
		cover (pBot);

	//TODO: silence/zoom using pEdict->v.button |= IN_ATTACK2;

	/* firearm usage */

	// TODO: this usage too narrow, should be smarter
	// about changing/reloading weapons. use ai_behavior
	// to accomplish this

	if (willFire (pBot) != 0) {
		openFire (pBot);
	} else if (pBot->threat == NULL && haveAmmo (pBot)) {
		pEdict->v.button |= IN_RELOAD;
	} else if (!ammoInClip (pBot) && haveAmmo (pBot)) {
		pEdict->v.button |= IN_RELOAD;
	} else if (!ammoInClip (pBot) && !haveAmmo (pBot)) {
		changeWeapon (pBot);
	}

	/* waypoints */

	if (num_waypoints != 0) {
		BotHeadTowardWaypoint (pBot);
	}

	if (validWaypoint (pBot->wp_current)) {
		if (waypoints[pBot->wp_current].flags & W_FL_CROUCH)
			pEdict->v.button |= IN_DUCK;

		if (waypoints[pBot->wp_current].flags & W_FL_LADDER) {
			// check if the waypoint is at the top of a ladder AND
			// the bot isn't currenly on a ladder...
			if (pBot->wp_ladder_top && pEdict->v.movetype != MOVETYPE_FLY) {
				// is the bot on "ground" above the ladder?
				if (pEdict->v.flags & FL_ONGROUND) {
					float waypoint_distance = (pEdict->v.origin - pBot->wp_origin).Length();

					if (waypoint_distance <= 20.0)  // if VERY close...
						pBot->move_speed = 20.0;  // go VERY slow
					else if (waypoint_distance <= 100.0)  // if fairly close...
						pBot->move_speed = 50.0;  // go fairly slow

					pBot->ladder_dir = LADDER_DOWN;
				} else { // bot must be in mid-air, go BACKWARDS to touch ladder...
					pBot->move_speed = -pBot->max_speed;
				}
			} else {
				pBot->wp_ladder_top = false;
			}
		}

		pBot->desiredVelocity = follow_wp (pBot, pBot->move_speed);
		pBot->lookAt = lookAhead (pBot);
	}

	/* look at threat */

	if (pBot->threat != NULL)
		pBot->lookAt = lookThreat (pBot);

	/* knife */

	if (pBot->threat != NULL && usingKnife (pBot) && inSight (pBot, pBot->threat)) {
		if (pEdict->v.button & IN_DUCK)
			pEdict->v.button ^= IN_DUCK; // do not duck
		pBot->move_speed = pBot->max_speed;
		pBot->desiredVelocity = seek (pEdict->v.origin, pBot->threat, pBot->move_speed);
	}

	/* pickup */

	if (pBot->threat == NULL && pBot->pickup != NULL && FVisible (pickupOrigin (pBot), pEdict))
		pBot->desiredVelocity = seek (pEdict->v.origin, pickupOrigin (pBot), pBot->move_speed);

	/* unaligned collision avoidance */

	Vector vel_sep = separate (pEdict, 32.0, pBot->move_speed);
	if (vel_sep != VEL_NOCHANGE)
		pBot->desiredVelocity = vel_sep;

	if (pBot->prev_speed >= 1.0 // desired
		&& distance (pBot->prev_origin, pEdict->v.origin) < 1.0 // actual travelled
		&& !randomize (120))
	{
		//speak (pBot, "STUCK");
		pEdict->v.button |= IN_JUMP;
	}

	/* movement overrides */

	if (pBot->plant_bomb_time > gpGlobals->time
		&& pBot->current_weapon.iId == CS_WEAPON_C4)
	{ // planting
		pBot->lookAt = lookPickup (pBot);
		pEdict->v.button |= IN_DUCK;
		pEdict->v.button |= IN_ATTACK;
		pBot->move_speed = 0;
	} else if (pBot->defuse_bomb_time > gpGlobals->time) { // defusing
		pBot->lookAt = lookPickup (pBot);
		pEdict->v.button |= IN_DUCK;
		pEdict->v.button |= IN_USE;
		pBot->move_speed = 0;
	} else if (pBot->pickup != NULL
		&& (isPlantedC4 (pBot->pickup) || isBombSite (pBot->pickup))
		&& distance (pEdict->v.origin, pickupOrigin (pBot)) < 50)
	{
		radio (pBot, RANDOM_FLOAT (0.0, 0.1), "1", "1"); // cover me
		pBot->lookAt = lookPickup (pBot);
		UTIL_SelectItem (pEdict, "weapon_c4");
		pBot->plant_bomb_time = gpGlobals->time + 5.0;
		pBot->defuse_bomb_time = gpGlobals->time + 12.0; // TODO: half if using defuse kit
	} else if (pBot->tactical_time > gpGlobals->time) {
		if (visibleWaypoint (pBot, pBot->wp_tactical)
			&& distance (waypoints[pBot->wp_tactical].origin, pEdict->v.origin) < WP_TOUCH_DISTANCE)
		{ // on top of wp_tactical
			pEdict->v.button |= IN_DUCK;
			pBot->move_speed = 0.0;
		} else { // enroute to wp_tactical
			if (pEdict->v.button & IN_DUCK)
				pEdict->v.button ^= IN_DUCK; // do not duck
			pBot->move_speed = pBot->max_speed;
		}
	}

	/* avoid obstacles */

	//pBot->desiredVelocity = avoid_obstacles (pBot, pBot->move_speed);

	/* execute movement */

	pEdict->v.v_angle = smoothLook (pBot);

	pEdict->v.v_angle = clampAngleTo180 (pEdict->v.v_angle);

	pEdict->v.angles.x = pEdict->v.v_angle.x / 3;
	pEdict->v.angles.y = pEdict->v.v_angle.y;
	pEdict->v.angles.z = 0;

	pEdict->v.v_angle.x = -pEdict->v.v_angle.x; // invert for engine

	Vector velAngle = smoothVelocity (pBot);
	float yaw_diff = pEdict->v.v_angle.y - velAngle.y;

	while (yaw_diff < 0) {
		yaw_diff += 360;
		// [0,oo)
	}

	while (yaw_diff >= 360) {
		yaw_diff -= 360;
		// [0,360)
	}

	float forwardPercent = yaw_diff;
	if (forwardPercent >= 180) {
		forwardPercent -= 180;
		// [0,180) -- fold to half-circle
	}

	if (forwardPercent > 90) {
		forwardPercent -= (forwardPercent - 90) * 2;
		// [0,90] w/ 90=>90 & 180=>0 -- fold to quarter circle
	}

	// goal: 90=>0 & 0=>1
	forwardPercent *= -1; // rotate
	forwardPercent += 90; // shift
	forwardPercent /= 90; // scale

	// reduce stuttering
	/*
	if (pBot->move_speed < 5.0)
		pBot->move_speed = 0.0;
	*/

	/* remember movement for next frame */
	pBot->prev_origin = pEdict->v.origin;
	pBot->prev_speed = pBot->move_speed;

	float move_forward = forwardPercent * pBot->move_speed;
	if (90.0 < yaw_diff && yaw_diff < 270.0) // backpedal
		move_forward *= -1.0;
	float move_strafe = pBot->move_speed - abs (move_forward);
	if (yaw_diff > 180.0) // left
		move_strafe *= -1.0;
	float move_vertical = 0.0; // never used

	g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, move_forward,
		move_strafe, move_vertical, pEdict->v.button, 0, pBot->msecval);
}
