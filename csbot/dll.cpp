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

$Source: /usr/local/cvsroot/csbot/src/dlls/dll.cpp,v $
$Revision: 1.12 $
$Date: 2001/05/11 01:43:01 $
$State: Exp $

***********************************

$Log: dll.cpp,v $
Revision 1.12  2001/05/11 01:43:01  eric
Added botkickall command

Revision 1.11  2001/05/11 00:34:21  eric
Added pathway pseudonym for pathwaypoint

Revision 1.10  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.9  2001/05/09 10:34:57  eric
botkillall command, NPE safeties

Revision 1.8  2001/05/09 05:03:39  eric
bugfixes from botman

Revision 1.7  2001/05/06 01:48:37  eric
Replaced global constants with enums where appropriate

Revision 1.6  2001/05/03 19:35:16  eric
CGF-style aiming

Revision 1.5  2001/04/30 08:06:11  eric
willFire() deals with crouching versus standing

Revision 1.4  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.3  2001/04/29 01:45:36  eric
Tweaked CVS info headers

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "ai.h"
#include "ai_terrain.h"
#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"


#define MENU_NONE  0
#define MENU_1     1
#define MENU_2     2
#define MENU_3     3

extern GETENTITYAPI other_GetEntityAPI;
extern enginefuncs_t g_engfuncs;
extern int debug_engine;
extern globalvars_t  *gpGlobals;
extern char *g_argv;
extern bool g_waypoint_on;
extern bool g_auto_waypoint;
extern bool g_path_waypoint;
extern int num_waypoints;
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern float wp_display_time[MAX_WAYPOINTS];
extern bot_t bots[MAX_BOTS];
extern bool b_observer_mode;
extern bool b_botdontshoot;
char welcome_msg[] = "CSbot - http://gamershomepage.com/csbot/\n";

static FILE *fp;

DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0,0,0);

int m_spriteTexture = 0;
int default_bot_skill = 50;
int isFakeClientCommand = 0;
int fake_arg_count;
float bot_check_time = 30.0;
int min_bots = -1;
int max_bots = -1;
int num_bots = 0;
int prev_num_bots = 0;
bool g_GameRules = FALSE;
edict_t *clients[32];
edict_t *listenserver_edict = NULL;
float welcome_time = 0.0;
bool welcome_sent = FALSE;
int g_menu_waypoint;
int g_menu_state = 0;

float is_team_play = 0.0;
char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
int num_teams = 0;
bool checked_teamplay = FALSE;

FILE *bot_cfg_fp = NULL;
bool need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
float respawn_time = 0.0;
bool spawn_time_reset = FALSE;

char bot_whine[MAX_BOT_WHINE][81];
int whine_count;
int recent_bot_whine[5];

cvar_t sv_bot = {"bot",""};

char *show_menu_1 =
   {"Waypoint Tags\n\n1. Wait for Lift\n2. Door\n3. More..."};
char *show_menu_3 =
   {"Waypoint Tags\n\n1. Bomb Site\n2. Rescue Area\n\n5. CANCEL"};


void BotNameInit(void);
void ProcessBotCfgFile(void);


void GameDLLInit( void )
{
   char filename[256];
   char buffer[256];
   int i, length;
   FILE *bfp;
   char *ptr;

   CVAR_REGISTER (&sv_bot);

   for (i=0; i<32; i++)
      clients[i] = NULL;

   whine_count = 0;

   // initialize the bots array of structures...
   memset(bots, 0, sizeof(bots));

   for (i=0; i < 5; i++)
      recent_bot_whine[i] = -1;

   BotNameInit();


   UTIL_BuildFileName(filename, "bot_whine.txt", NULL);

   bfp = fopen(filename, "r");

   if (bfp != NULL)
   {
      while ((whine_count < MAX_BOT_WHINE) &&
             (fgets(buffer, 80, bfp) != NULL))
      {
         length = strlen(buffer);

         if (buffer[length-1] == '\n')
         {
            buffer[length-1] = 0;  // remove '\n'
            length--;
         }

         if ((ptr = strstr(buffer, "%n")) != NULL)
         {
            *(ptr+1) = 's';  // change %n to %s
         }

         if (length > 0)
         {
            strcpy(bot_whine[whine_count], buffer);
            whine_count++;
         }
      }

      fclose(bfp);
   }

   (*other_gFunctionTable.pfnGameInit)();
}

int DispatchSpawn( edict_t *pent )
{
   if (gpGlobals->deathmatch)
   {
      char *pClassname = (char *)STRING(pent->v.classname);

      if (debug_engine)
      {
         fp=fopen("bot.txt","a");
         fprintf(fp, "DispatchSpawn: %x %s\n",pent,pClassname);
         if (pent->v.model != 0)
            fprintf(fp, " model=%s\n",STRING(pent->v.model));
         fclose(fp);
      }

      if (strcmp(pClassname, "worldspawn") == 0)
      {
         // do level initialization stuff here...

         WaypointInit();
         WaypointLoad(NULL);

         PRECACHE_SOUND("weapons/xbow_hit1.wav");      // waypoint add
         PRECACHE_SOUND("weapons/mine_activate.wav");  // waypoint delete
         PRECACHE_SOUND("common/wpn_hudoff.wav");      // path add/delete start
         PRECACHE_SOUND("common/wpn_hudon.wav");       // path add/delete done
         PRECACHE_SOUND("common/wpn_moveselect.wav");  // path add/delete cancel
         PRECACHE_SOUND("common/wpn_denyselect.wav");  // path add/delete error

         m_spriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr");

         g_GameRules = TRUE;

         is_team_play = 0.0;
         memset(team_names, 0, sizeof(team_names));
         num_teams = 0;
         checked_teamplay = FALSE;

         bot_cfg_pause_time = 0.0;
         respawn_time = 0.0;
         spawn_time_reset = FALSE;

         prev_num_bots = num_bots;
         num_bots = 0;

         bot_check_time = gpGlobals->time + 30.0;
      }
   }

   return (*other_gFunctionTable.pfnSpawn)(pent);
}

void DispatchThink( edict_t *pent )
{
   (*other_gFunctionTable.pfnThink)(pent);
}

void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnUse)(pentUsed, pentOther);
}

void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
   (*other_gFunctionTable.pfnBlocked)(pentBlocked, pentOther);
}

void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
   (*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
}

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnSave)(pent, pSaveData);
}

int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
   return (*other_gFunctionTable.pfnRestore)(pent, pSaveData, globalEntity);
}

void DispatchObjectCollsionBox( edict_t *pent )
{
   (*other_gFunctionTable.pfnSetAbsBox)(pent);
}

void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
   (*other_gFunctionTable.pfnSaveWriteFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
   (*other_gFunctionTable.pfnSaveReadFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnSaveGlobalState)(pSaveData);
}

void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
   (*other_gFunctionTable.pfnRestoreGlobalState)(pSaveData);
}

void ResetGlobalState( void )
{
   (*other_gFunctionTable.pfnResetGlobalState)();
}

BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{ 
   if (gpGlobals->deathmatch)
   {
      int i;
      int count = 0;

      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp, "ClientConnect: pent=%x name=%s\n",pEntity,pszName); fclose(fp); }

      // check if this client is the listen server client
      if (strcmp(pszAddress, "loopback") == 0)
      {
         // save the edict of the listen server client...
         listenserver_edict = pEntity;
      }

      // check if this is NOT a bot joining the server...
      if (strcmp(pszAddress, "127.0.0.1") != 0)
      {
         // don't try to add bots for 60 seconds, give client time to get added
         bot_check_time = gpGlobals->time + 60.0;

         for (i=0; i < MAX_BOTS; i++)
         {
            if (bots[i].is_used)  // count the number of bots in use
               count++;
         }

         // if there are currently more than the minimum number of bots running
         // then kick one of the bots off the server...
         if ((count > min_bots) && (min_bots != -1))
         {
            for (i=0; i < MAX_BOTS; i++)
            {
               if (bots[i].is_used)  // is this slot used?
               {
                  char cmd[80];

                  sprintf(cmd, "kick \"%s\"\n", bots[i].name);

                  SERVER_COMMAND(cmd);  // kick the bot using (kick "name")

                  break;
               }
            }
         }
      }
   }

   return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
}

void ClientDisconnect( edict_t *pEntity )
{
   if (gpGlobals->deathmatch)
   {
      int i;

      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp, "ClientDisconnect: %x\n",pEntity); fclose(fp); }

      i = 0;
      while ((i < 32) && (clients[i] != pEntity))
         i++;

      if (i < 32)
         clients[i] = NULL;


      for (i=0; i < MAX_BOTS; i++)
      {
         if (bots[i].pEdict == pEntity)
         {
            // someone kicked this bot off of the server...

            bots[i].is_used = FALSE;  // this slot is now free to use

            bots[i].kick_time = gpGlobals->time;  // save the kicked time

            break;
         }
      }
   }

   (*other_gFunctionTable.pfnClientDisconnect)(pEntity);
}

void ClientKill( edict_t *pEntity )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp, "ClientKill: %x\n",pEntity); fclose(fp); }
   (*other_gFunctionTable.pfnClientKill)(pEntity);
}

void ClientPutInServer( edict_t *pEntity )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp, "ClientPutInServer: %x\n",pEntity); fclose(fp); }

   int i = 0;

   while ((i < 32) && (clients[i] != NULL))
      i++;

   if (i < 32)
      clients[i] = pEntity;  // store this clients edict in the clients array

   (*other_gFunctionTable.pfnClientPutInServer)(pEntity);
}

void ClientCommand( edict_t *pEntity )
{
   // only allow custom commands if deathmatch mode and NOT dedicated server and
   // client sending command is the listen server client...

   if ((gpGlobals->deathmatch) && (!IS_DEDICATED_SERVER()) &&
       (pEntity == listenserver_edict))
   {
      const char *pcmd = Cmd_Argv(0);
      const char *arg1 = Cmd_Argv(1);
      const char *arg2 = Cmd_Argv(2);
      const char *arg3 = Cmd_Argv(3);
      const char *arg4 = Cmd_Argv(4);
      char msg[80];

      if (debug_engine)
      {
         fp=fopen("bot.txt","a"); fprintf(fp,"ClientCommand: %s",pcmd);
         if ((arg1 != NULL) && (*arg1 != 0))
            fprintf(fp," %s", arg1);
         if ((arg2 != NULL) && (*arg2 != 0))
            fprintf(fp," %s", arg2);
         if ((arg3 != NULL) && (*arg3 != 0))
            fprintf(fp," %s", arg3);
         if ((arg4 != NULL) && (*arg4 != 0))
            fprintf(fp," %s", arg4);
         fprintf(fp, "\n");
         fclose(fp);
      }

      if (FStrEq(pcmd, "addbot"))
      {
         BotCreate( pEntity, arg1, arg2, arg3, arg4 );

         bot_check_time = gpGlobals->time + 5.0;

         return;
      }
      else if (FStrEq(pcmd, "observer"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);
            if (temp)
               b_observer_mode = TRUE;
            else
               b_observer_mode = FALSE;
         }

         if (b_observer_mode)
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

         return;
      }
      else if (FStrEq(pcmd, "botskill"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);

            if ((temp < 1) || (temp > 100))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid botskill value!\n");
            else
               default_bot_skill = temp;
         }

         sprintf(msg, "botskill is %d\n", default_bot_skill);
         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "botdontshoot"))
      {
         if ((arg1 != NULL) && (*arg1 != 0))
         {
            int temp = atoi(arg1);
            if (temp)
               b_botdontshoot = true;
            else
               b_botdontshoot = false;
         }

         if (b_botdontshoot)
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot ENABLED\n");
         else
            ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot DISABLED\n");

         return;
      }
      else if (FStrEq(pcmd, "botkickall"))
      {
         for (int i = 0; i < MAX_BOTS; i++) {
			 bot_t *bot = &bots[i];
			 if (bot != NULL && bot->is_used) {
				char cmd[80];
				sprintf (cmd, "kick \"%s\"\n", bot->name);
				SERVER_COMMAND (cmd);  // kick the bot using (kick "name")
            }
         }

         return;
      }
      else if (FStrEq(pcmd, "botkillall"))
      {
         for (int i = 0; i < MAX_BOTS; i++) {
			 bot_t *bot = &bots[i];
			 if (bot != NULL && bot->is_used && bot->pEdict != NULL) {
				 ClientKill (bot->pEdict);
            }
         }

         return;
      }/*
	  else if (FStrEq(pcmd, "botstatus"))
      {
		  if (arg1 != NULL) {
			for (int m = 1; m <= gpGlobals->maxClients; m++) {
				edict_t *client = INDEXENT (m);
				bot_t *bot = UTIL_GetBotPointer (client);

				if (bot == NULL || client == NULL || client->free || !IsAlive (client))
					continue;

				if (FStrEq (arg1, STRING (client->v.netname))) {
					char botstatus_msg[400];
					float speed = (client->v.velocity).Length ();
/*
   Vector desiredVelocity;
   Vector lookAt;
   edict_t *pickup;
   edict_t *threat;

   squad_t *pSquad;
   MOVEMENT movement_style;
   SQUAD_POSITION squad_position;
   SQUAD_ROLE squad_role;
*
					sprintf (botstatus_msg,"botstatus %s: speed=X%f/O%f/V%f, wp=L%n/C%n/T%n, \n",
						STRING (client->v.netname),
						bot->max_speed, bot->move_speed, speed,
						bot->wp_last, bot->wp_current, bot->wp_tactical);
					ClientPrint (pEntity, HUD_PRINTNOTIFY, botstatus_msg);
					break;
				}
			}
		  }
	  }
	  else if (FStrEq(pcmd, "showpath"))
	  {
		  if (arg1 != NULL && arg2 != NULL) {
			  ClientPrint(pEntity, HUD_PRINTNOTIFY, "showing path");
		  }
	  }*/
	  else if (FStrEq(pcmd, "myspeed"))
	  {
		  float my_speed = (pEntity->v.velocity).Length ();
		  char msg[100];
		  sprintf (msg, "Current speed: %f\n", my_speed);
		  ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
	  }
      else if (FStrEq(pcmd, "debug_engine"))
      {
         debug_engine = 1;

         ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine enabled!\n");

         return;
      }
      else if (FStrEq(pcmd, "waypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_waypoint_on = TRUE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_waypoint_on = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }
         else if (FStrEq(arg1, "add"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointAdd(pEntity);
         }
         else if (FStrEq(arg1, "delete"))
         {
            if (!g_waypoint_on)
               g_waypoint_on = TRUE;  // turn waypoints on if off

            WaypointDelete(pEntity);
         }
         else if (FStrEq(arg1, "save"))
         {
            WaypointSave();

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints saved\n");
         }
         else if (FStrEq(arg1, "load"))
         {
            if (WaypointLoad(pEntity))
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");
         }
		 else if (FStrEq(arg1, "terrain"))
		 {
			TerrainInitAll();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "terrain loaded\n");
		 }
         else if (FStrEq(arg1, "menu"))
         {
            int index;

            if (num_waypoints < 1)
               return;

            index = WaypointFindNearest(pEntity, 50.0);

            if (index == -1)
               return;

            g_menu_waypoint = index;
            g_menu_state = MENU_1;

            UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_1);
         }
         else if (FStrEq(arg1, "info"))
         {
            WaypointPrintInfo(pEntity);
         }
         else
         {
            if (g_waypoint_on)
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
            else
               ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
         }

         return;
      }
      else if (FStrEq(pcmd, "autowaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_auto_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case
         }
         else if (FStrEq(arg1, "off"))
         {
            g_auto_waypoint = FALSE;
         }

         if (g_auto_waypoint)
            sprintf(msg, "autowaypoint is ON\n");
         else
            sprintf(msg, "autowaypoint is OFF\n");

         ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "pathway") || FStrEq(pcmd, "pathwaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_path_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathway is ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_path_waypoint = FALSE;

            ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathway is OFF\n");
         }
         else if (FStrEq(arg1, "create1"))
         {
            WaypointCreatePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "create2"))
         {
            WaypointCreatePath(pEntity, 2);
         }
         else if (FStrEq(arg1, "remove1"))
         {
            WaypointRemovePath(pEntity, 1);
         }
         else if (FStrEq(arg1, "remove2"))
         {
            WaypointRemovePath(pEntity, 2);
         }

         return;
      }
      else if (FStrEq(pcmd, "menuselect") && (g_menu_state != MENU_NONE))
      {
         if (g_menu_state == MENU_1)  // main menu...
         {
            if (FStrEq(arg1, "1"))  // wait for lift...
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_LIFT)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_LIFT;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_LIFT;  // on
            }
            else if (FStrEq(arg1, "2"))  // door waypoint
            {
               if (waypoints[g_menu_waypoint].flags & W_FL_DOOR)
                  waypoints[g_menu_waypoint].flags &= ~W_FL_DOOR;  // off
               else
                  waypoints[g_menu_waypoint].flags |= W_FL_DOOR;  // on
            }
            else if (FStrEq(arg1, "3"))  // more...
            {
               g_menu_state = MENU_3;

               UTIL_ShowMenu(pEntity, 0x13, -1, FALSE, show_menu_3);

               return;
            }
         }
         else if (g_menu_state == MENU_3)  // second menu...
         {
               if (FStrEq(arg1, "1"))  // flag location
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_BOMB_SITE)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_BOMB_SITE;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_BOMB_SITE;  // on
               }
               else if (FStrEq(arg1, "2"))  // flag goal
               {
                  if (waypoints[g_menu_waypoint].flags & W_FL_RESCUE_ZONE)
                     waypoints[g_menu_waypoint].flags &= ~W_FL_RESCUE_ZONE;  // off
                  else
                     waypoints[g_menu_waypoint].flags |= W_FL_RESCUE_ZONE;  // on
               }
         }

         g_menu_state = MENU_NONE;

         return;
      }
      else if (FStrEq(pcmd, "search"))
      {
         edict_t *pent = NULL;
         float radius = 50;
         char str[80];

         ClientPrint(pEntity, HUD_PRINTCONSOLE, "searching...\n");

         while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
         {
            sprintf(str, "Found %s at %5.2f %5.2f %5.2f\n",
                       STRING(pent->v.classname),
                       pent->v.origin.x, pent->v.origin.y,
                       pent->v.origin.z);
            ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

            FILE *fp=fopen("bot.txt", "a");
            fprintf(fp, "ClientCommmand: search %s", str);
            fclose(fp);
         }

         return;
      }
   }

   (*other_gFunctionTable.pfnClientCommand)(pEntity);
}

void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
   if (debug_engine) { fp=fopen("bot.txt", "a"); fprintf(fp, "ClientUserInfoChanged: pEntity=%x infobuffer=%s\n", pEntity, infobuffer); fclose(fp); }

   (*other_gFunctionTable.pfnClientUserInfoChanged)(pEntity, infobuffer);
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
   (*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);
}

void PlayerPreThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnPlayerPreThink)(pEntity);
}

void PlayerPostThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnPlayerPostThink)(pEntity);
}

void StartFrame( void )
{
   if (gpGlobals->deathmatch)
   {
      edict_t *pPlayer;
      static float check_server_cmd = 0.0;
      static int i, index, player_index, bot_index;
      static float previous_time = -1.0;
      char msg[256];
      int count;

      // if a new map has started then (MUST BE FIRST IN StartFrame)...
      if ((gpGlobals->time + 0.1) < previous_time)
      {
         char filename[256];
         char mapname[64];

         check_server_cmd = 0.0;  // reset at start of map

         // check if mapname_bot.cfg file exists...

         strcpy(mapname, STRING(gpGlobals->mapname));
         strcat(mapname, "_bot.cfg");

         UTIL_BuildFileName(filename, "maps", mapname);

         if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
         {
            sprintf(msg, "Executing %s\n", filename);
            ALERT( at_console, msg );

            for (index = 0; index < MAX_BOTS; index++)
            {
               bots[index].is_used = FALSE;
               bots[index].respawn_state = RESPAWN_UNKNOWN;
               bots[index].kick_time = 0.0;
            }

            if (IS_DEDICATED_SERVER())
               bot_cfg_pause_time = gpGlobals->time + 5.0;
            else
               bot_cfg_pause_time = gpGlobals->time + 20.0;
         }
         else
         {
            count = 0;

            // mark the bots as needing to be respawned...
            for (index = 0; index < 32; index++)
            {
               if (count >= prev_num_bots)
               {
                  bots[index].is_used = FALSE;
                  bots[index].respawn_state = RESPAWN_UNKNOWN;
                  bots[index].kick_time = 0.0;
               }

               if (bots[index].is_used)  // is this slot used?
               {
                  bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
                  count++;
               }

               // check for any bots that were very recently kicked...
               if ((bots[index].kick_time + 5.0) > previous_time)
               {
                  bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
                  count++;
               }
               else
                  bots[index].kick_time = 0.0;  // reset to prevent false spawns later
            }

            // set the respawn time
            if (IS_DEDICATED_SERVER())
               respawn_time = gpGlobals->time + 5.0;
            else
               respawn_time = gpGlobals->time + 20.0;
         }

         bot_check_time = gpGlobals->time + 30.0;
      }

      if (!IS_DEDICATED_SERVER())
      {
         if ((listenserver_edict != NULL) && (welcome_sent == FALSE) &&
             (welcome_time < 1.0))
         {
            // are they out of observer mode yet?
            if (IsAlive(listenserver_edict))
               welcome_time = gpGlobals->time + 5.0;  // welcome in 5 seconds
         }

         if ((welcome_time > 0.0) && (welcome_time < gpGlobals->time) &&
             (welcome_sent == FALSE))
         {
            // let's send a welcome message to this client...
            UTIL_SayText(welcome_msg, listenserver_edict);

            welcome_sent = TRUE;  // clear this so we only do it once
         }
      }

      count = 0;

      for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
      {
         if ((bots[bot_index].is_used) &&  // is this slot used AND
             (bots[bot_index].respawn_state == RESPAWN_IDLE))  // not respawning
         {
            BotThink(&bots[bot_index]);

            count++;
         }
      }

	  check_bomb ();

      if (count > num_bots)
         num_bots = count;

      for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
      {
         pPlayer = INDEXENT(player_index);

         if (pPlayer && !pPlayer->free)
         {
			// botman's bugfix. thank you!
            if (g_waypoint_on
				&& FBitSet(pPlayer->v.flags, FL_CLIENT)
				&& (!FBitSet(pPlayer->v.flags, FL_FAKECLIENT)))
            {
                  WaypointThink(pPlayer);
            }
         }
      }

      // are we currently respawning bots and is it time to spawn one yet?
      if ((respawn_time > 1.0) && (respawn_time <= gpGlobals->time))
      {
         int index = 0;

         // find bot needing to be respawned...
         while ((index < MAX_BOTS) &&
                (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
            index++;

         if (index < MAX_BOTS)
         {
            bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
            bots[index].is_used = FALSE;      // free up this slot

            // respawn 1 bot then wait a while (otherwise engine crashes)
            char c_skill[2];
            char c_team[2];
            char c_class[3];

            sprintf(c_skill, "%d", bots[index].bot_skill);
            sprintf(c_team, "%d", bots[index].bot_team);
            sprintf(c_class, "%d", bots[index].bot_class);

            BotCreate(NULL, c_team, c_class, bots[index].name, c_skill);

            respawn_time = gpGlobals->time + 2.0;  // set next respawn time

            bot_check_time = gpGlobals->time + 5.0;
         }
         else
         {
            respawn_time = 0.0;
         }
      }

      if (g_GameRules)
      {
         if (need_to_open_cfg)  // have we open bot.cfg file yet?
         {
            char filename[256];
            char mapname[64];

            need_to_open_cfg = FALSE;  // only do this once!!!

            // check if mapname_bot.cfg file exists...

            strcpy(mapname, STRING(gpGlobals->mapname));
            strcat(mapname, "_bot.cfg");

            UTIL_BuildFileName(filename, "maps", mapname);

            if ((bot_cfg_fp = fopen(filename, "r")) != NULL)
            {
               sprintf(msg, "Executing %s\n", filename);
               ALERT( at_console, msg );
            }
            else
            {
               UTIL_BuildFileName(filename, "bot.cfg", NULL);

               sprintf(msg, "Executing %s\n", filename);
               ALERT( at_console, msg );

               bot_cfg_fp = fopen(filename, "r");

               if (bot_cfg_fp == NULL)
                  ALERT( at_console, "bot.cfg file not found\n" );
            }

            if (IS_DEDICATED_SERVER())
               bot_cfg_pause_time = gpGlobals->time + 5.0;
            else
               bot_cfg_pause_time = gpGlobals->time + 20.0;
         }

         if (!IS_DEDICATED_SERVER() && !spawn_time_reset)
         {
            if (listenserver_edict != NULL)
            {
               if (IsAlive(listenserver_edict))
               {
                  spawn_time_reset = TRUE;

                  if (respawn_time >= 1.0)
                     respawn_time = min(respawn_time, gpGlobals->time + (float)1.0);

                  if (bot_cfg_pause_time >= 1.0)
                     bot_cfg_pause_time = min(bot_cfg_pause_time, gpGlobals->time + (float)1.0);
               }
            }
         }

         if ((bot_cfg_fp) &&
             (bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= gpGlobals->time))
         {
            // process bot.cfg file options...
            ProcessBotCfgFile();
         }

      }      

      // if time to check for server commands then do so...
      if ((check_server_cmd <= gpGlobals->time) && IS_DEDICATED_SERVER())
      {
         check_server_cmd = gpGlobals->time + 1.0;

         char *cvar_bot = (char *)CVAR_GET_STRING( "bot" );

         if ( cvar_bot && cvar_bot[0] )
         {
            char cmd_line[80];
            char *cmd, *arg1, *arg2, *arg3, *arg4;

            strcpy(cmd_line, cvar_bot);

            index = 0;
            cmd = cmd_line;
            arg1 = arg2 = arg3 = arg4 = NULL;

            // skip to blank or end of string...
            while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
               index++;

            if (cmd_line[index] == ' ')
            {
               cmd_line[index++] = 0;
               arg1 = &cmd_line[index];

               // skip to blank or end of string...
               while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                  index++;

               if (cmd_line[index] == ' ')
               {
                  cmd_line[index++] = 0;
                  arg2 = &cmd_line[index];

                  // skip to blank or end of string...
                  while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                     index++;

                  if (cmd_line[index] == ' ')
                  {
                     cmd_line[index++] = 0;
                     arg3 = &cmd_line[index];

                     // skip to blank or end of string...
                     while ((cmd_line[index] != ' ') && (cmd_line[index] != 0))
                        index++;

                     if (cmd_line[index] == ' ')
                     {
                        cmd_line[index++] = 0;
                        arg4 = &cmd_line[index];
                     }
                  }
               }
            }

            if (strcmp(cmd, "addbot") == 0)
            {
               BotCreate( NULL, arg1, arg2, arg3, arg4 );

               bot_check_time = gpGlobals->time + 5.0;
            }
            else if (strcmp(cmd, "min_bots") == 0)
            {
               min_bots = atoi( arg1 );

               if ((min_bots < 0) || (min_bots > 31))
                  min_bots = 1;
      
               sprintf(msg, "min_bots set to %d\n", min_bots);
               printf(msg);
            }
            else if (strcmp(cmd, "max_bots") == 0)
            {
               max_bots = atoi( arg1 );
     
               if ((max_bots < 0) || (max_bots > 31)) 
                  max_bots = 1;

               sprintf(msg, "max_bots set to %d\n", max_bots);
               printf(msg);
            }

            CVAR_SET_STRING("bot", "");
         }
      }

      // check if time to see if a bot needs to be created...
      if (bot_check_time < gpGlobals->time)
      {
         int count = 0;

         bot_check_time = gpGlobals->time + 5.0;

         for (i = 0; i < 32; i++)
         {
            if (clients[i] != NULL)
               count++;
         }

         // if there are currently less than the maximum number of "players"
         // then add another bot using the default skill level...
         if ((count < max_bots) && (max_bots != -1))
         {
            BotCreate( NULL, NULL, NULL, NULL, NULL );
         }
      }

      previous_time = gpGlobals->time;
   }

   (*other_gFunctionTable.pfnStartFrame)();
}

void ParmsNewLevel( void )
{
   (*other_gFunctionTable.pfnParmsNewLevel)();
}

void ParmsChangeLevel( void )
{
   (*other_gFunctionTable.pfnParmsChangeLevel)();
}

const char *GetGameDescription( void )
{
   return (*other_gFunctionTable.pfnGetGameDescription)();
}

void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
   if (debug_engine) { fp=fopen("bot.txt", "a"); fprintf(fp, "PlayerCustomization: %x\n",pEntity); fclose(fp); }

   (*other_gFunctionTable.pfnPlayerCustomization)(pEntity, pCust);
}

void SpectatorConnect( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorConnect)(pEntity);
}

void SpectatorDisconnect( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorDisconnect)(pEntity);
}

void SpectatorThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnSpectatorThink)(pEntity);
}


DLL_FUNCTIONS gFunctionTable =
{
   GameDLLInit,               //pfnGameInit
   DispatchSpawn,             //pfnSpawn
   DispatchThink,             //pfnThink
   DispatchUse,               //pfnUse
   DispatchTouch,             //pfnTouch
   DispatchBlocked,           //pfnBlocked
   DispatchKeyValue,          //pfnKeyValue
   DispatchSave,              //pfnSave
   DispatchRestore,           //pfnRestore
   DispatchObjectCollsionBox, //pfnAbsBox

   SaveWriteFields,           //pfnSaveWriteFields
   SaveReadFields,            //pfnSaveReadFields

   SaveGlobalState,           //pfnSaveGlobalState
   RestoreGlobalState,        //pfnRestoreGlobalState
   ResetGlobalState,          //pfnResetGlobalState

   ClientConnect,             //pfnClientConnect
   ClientDisconnect,          //pfnClientDisconnect
   ClientKill,                //pfnClientKill
   ClientPutInServer,         //pfnClientPutInServer
   ClientCommand,             //pfnClientCommand
   ClientUserInfoChanged,     //pfnClientUserInfoChanged
   ServerActivate,            //pfnServerActivate

   PlayerPreThink,            //pfnPlayerPreThink
   PlayerPostThink,           //pfnPlayerPostThink

   StartFrame,                //pfnStartFrame
   ParmsNewLevel,             //pfnParmsNewLevel
   ParmsChangeLevel,          //pfnParmsChangeLevel

   GetGameDescription,        //pfnGetGameDescription    Returns string describing current .dll game.
   PlayerCustomization,       //pfnPlayerCustomization   Notifies .dll of new customization for player.

   SpectatorConnect,          //pfnSpectatorConnect      Called when spectator joins server
   SpectatorDisconnect,       //pfnSpectatorDisconnect   Called when spectator leaves the server
   SpectatorThink,            //pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)
};

#ifdef __BORLANDC__
int EXPORT GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
#else
extern "C" EXPORT int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
#endif
{
   // check if engine's pointer is valid and version is correct...

   if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
      return FALSE;

   // pass engine callback function table to engine...
   memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );

   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION))
   {
      return FALSE;  // error initializing function table!!!
   }

   return TRUE;
}


void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3)
{
   int length;

   memset(g_argv, 0, sizeof(g_argv));

   isFakeClientCommand = 1;

   if ((arg1 == NULL) || (*arg1 == 0))
      return;

   if ((arg2 == NULL) || (*arg2 == 0))
   {
      length = sprintf(&g_argv[0], "%s", arg1);
      fake_arg_count = 1;
   }
   else if ((arg3 == NULL) || (*arg3 == 0))
   {
      length = sprintf(&g_argv[0], "%s %s", arg1, arg2);
      fake_arg_count = 2;
   }
   else
   {
      length = sprintf(&g_argv[0], "%s %s %s", arg1, arg2, arg3);
      fake_arg_count = 3;
   }

   g_argv[length] = 0;  // null terminate just in case

   strcpy(&g_argv[64], arg1);

   if (arg2)
      strcpy(&g_argv[128], arg2);

   if (arg3)
      strcpy(&g_argv[192], arg3);

   // allow the MOD DLL to execute the ClientCommand...
   ClientCommand(pBot);

   isFakeClientCommand = 0;
}


const char *Cmd_Args( void )
{
   if (isFakeClientCommand)
   {
      return &g_argv[0];
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Args)();
   }
}


const char *Cmd_Argv( int argc )
{
   if (isFakeClientCommand)
   {
      if (argc == 0)
      {
         return &g_argv[64];
      }
      else if (argc == 1)
      {
         return &g_argv[128];
      }
      else if (argc == 2)
      {
         return &g_argv[192];
      }
      else
      {
         return NULL;
      }
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Argv)(argc);
   }
}


int Cmd_Argc( void )
{
   if (isFakeClientCommand)
   {
      return fake_arg_count;
   }
   else
   {
      return (*g_engfuncs.pfnCmd_Argc)();
   }
}


void ProcessBotCfgFile(void)
{
   int ch;
   char cmd_line[256];
   int cmd_index;
   static char server_cmd[80];
   char *cmd, *arg1, *arg2, *arg3, *arg4;
   char msg[80];

   if (bot_cfg_pause_time > gpGlobals->time)
      return;

   if (bot_cfg_fp == NULL)
      return;

   cmd_index = 0;
   cmd_line[cmd_index] = 0;

   ch = fgetc(bot_cfg_fp);

   // skip any leading blanks
   while (ch == ' ')
      ch = fgetc(bot_cfg_fp);

   while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
   {
      if (ch == '\t')  // convert tabs to spaces
         ch = ' ';

      cmd_line[cmd_index] = ch;

      ch = fgetc(bot_cfg_fp);

      // skip multiple spaces in input file
      while ((cmd_line[cmd_index] == ' ') &&
             (ch == ' '))      
         ch = fgetc(bot_cfg_fp);

      cmd_index++;
   }

   if (ch == '\r')  // is it a carriage return?
   {
      ch = fgetc(bot_cfg_fp);  // skip the linefeed
   }

   // if reached end of file, then close it
   if (ch == EOF)
   {
      fclose(bot_cfg_fp);

      bot_cfg_fp = NULL;

      bot_cfg_pause_time = 0.0;
   }

   cmd_line[cmd_index] = 0;  // terminate the command line

   // copy the command line to a server command buffer...
   strcpy(server_cmd, cmd_line);
   strcat(server_cmd, "\n");

   cmd_index = 0;
   cmd = cmd_line;
   arg1 = arg2 = arg3 = arg4 = NULL;

   // skip to blank or end of string...
   while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
      cmd_index++;

   if (cmd_line[cmd_index] == ' ')
   {
      cmd_line[cmd_index++] = 0;
      arg1 = &cmd_line[cmd_index];

      // skip to blank or end of string...
      while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
         cmd_index++;

      if (cmd_line[cmd_index] == ' ')
      {
         cmd_line[cmd_index++] = 0;
         arg2 = &cmd_line[cmd_index];

         // skip to blank or end of string...
         while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
            cmd_index++;

         if (cmd_line[cmd_index] == ' ')
         {
            cmd_line[cmd_index++] = 0;
            arg3 = &cmd_line[cmd_index];

            // skip to blank or end of string...
            while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
               cmd_index++;

            if (cmd_line[cmd_index] == ' ')
            {
               cmd_line[cmd_index++] = 0;
               arg4 = &cmd_line[cmd_index];
            }
         }
      }
   }

   if ((cmd_line[0] == '#') || (cmd_line[0] == 0))
      return;  // return if comment or blank line

   if (strcmp(cmd, "addbot") == 0)
   {
      BotCreate( NULL, arg1, arg2, arg3, arg4 );

      // have to delay here or engine gives "Tried to write to
      // uninitialized sizebuf_t" error and crashes...

      bot_cfg_pause_time = gpGlobals->time + 2.0;
      bot_check_time = gpGlobals->time + 5.0;

      return;
   }

   if (strcmp(cmd, "botskill") == 0)
   {
      int temp = atoi(arg1);

      if ((temp >= 1) && (temp <= 100))
         default_bot_skill = atoi( arg1 );  // set default bot skill level

      return;
   }

   if (strcmp(cmd, "observer") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_observer_mode = TRUE;
      else
         b_observer_mode = FALSE;

      return;
   }

   if (strcmp(cmd, "botdontshoot") == 0)
   {
      int temp = atoi(arg1);

      if (temp)
         b_botdontshoot = true;
      else
         b_botdontshoot = false;

      return;
   }

   if (strcmp(cmd, "min_bots") == 0)
   {
      min_bots = atoi( arg1 );

      if ((min_bots < 0) || (min_bots > 31))
         min_bots = 1;

      if (IS_DEDICATED_SERVER())
      {
         sprintf(msg, "min_bots set to %d\n", min_bots);
         printf(msg);
      }

      return;
   }

   if (strcmp(cmd, "max_bots") == 0)
   {
      max_bots = atoi( arg1 );

      if ((max_bots < 0) || (max_bots > 31)) 
         max_bots = 1;

      if (IS_DEDICATED_SERVER())
      {
         sprintf(msg, "max_bots set to %d\n", max_bots);
         printf(msg);
      }

      return;
   }

   if (strcmp(cmd, "pause") == 0)
   {
      bot_cfg_pause_time = gpGlobals->time + atoi( arg1 );

      return;
   }

   sprintf(msg, "executing server command: %s\n", server_cmd);
   ALERT( at_console, msg );

   if (IS_DEDICATED_SERVER())
      printf(msg);

   SERVER_COMMAND(server_cmd);
}

