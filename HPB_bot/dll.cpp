//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// dll.cpp
//

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"


extern GETENTITYAPI other_GetEntityAPI;
extern enginefuncs_t g_engfuncs;
extern int debug_engine;
extern globalvars_t  *gpGlobals;
extern char *g_argv;
extern bool g_waypoint_on;
extern bool g_auto_waypoint;
extern bool g_path_waypoint;
extern int num_waypoints;  // number of waypoints currently in use
extern WAYPOINT waypoints[MAX_WAYPOINTS];
extern float wp_display_time[MAX_WAYPOINTS];
extern bot_t bots[32];

static FILE *fp;

DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0,0,0);

int mod_id = 0;
int m_spriteTexture = 0;
int isFakeClientCommand = 0;
int fake_arg_count;
float bot_check_time = 10.0;
int min_bots = -1;
int max_bots = -1;
bool g_GameRules = FALSE;
edict_t *clients[32];
float welcome_time = 0.0;
int welcome_index = -1;
bool welcome_sent = FALSE;

cvar_t sv_bot = {"bot",""};
char welcome_msg[] = "HPB bot - http://planethalflife.com/botman\n";


void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);


extern void welcome_init(void);

void GameDLLInit( void )
{
   CVAR_REGISTER (&sv_bot);

   for (int i=0; i<32; i++)
      clients[i] = NULL;

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
         fprintf(fp, "DispatchSpawn: %s\n",pClassname);
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

      i = 0;
      while ((i < 32) && (clients[i] != NULL))
         i++;

      if (i < 32)
         clients[i] = pEntity;

      // check if this is NOT a bot joining the server...
      if (strcmp(pszAddress, "127.0.0.1") != 0)
      {
         if (welcome_index == -1)
            welcome_index = i;

         // don't try to add bots for 30 seconds, give client time to get added
         bot_check_time = gpGlobals->time + 30.0;

         for (i=0; i < 32; i++)
         {
            if (bots[i].is_used)  // count the number of bots in use
               count++;
         }

         // if there are currently more than the minimum number of bots running
         // then kick one of the bots off the server...
         if ((count > min_bots) && (min_bots != -1))
         {
            for (i=0; i < 32; i++)
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
      int i, index;

      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp, "ClientDisconnect: %x\n",pEntity); fclose(fp); }

      i = 0;
      while ((i < 32) && (clients[i] != pEntity))
         i++;

      if (i < 32)
         clients[i] = NULL;

      index = -1;

      for (i=0; i < 32; i++)
      {
         if (bots[i].pEdict == pEntity)
         {
            index = i;
            break;
         }
      }

      if (index != -1)
      {
         // someone kicked this bot off of the server...

         bots[index].is_used = FALSE;  // this slot is now free to use
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
   (*other_gFunctionTable.pfnClientPutInServer)(pEntity);
}

void ClientCommand( edict_t *pEntity )
{
   // these client commands aren't allow in single player mode or
   // on dedicated servers

   if ((gpGlobals->deathmatch) && (!IS_DEDICATED_SERVER()))
   {
      const char *pcmd = Cmd_Argv(0);
      const char *arg1 = Cmd_Argv(1);
      const char *arg2 = Cmd_Argv(2);
      const char *arg3 = Cmd_Argv(3);
      const char *arg4 = Cmd_Argv(4);
      char msg[80];

      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"ClientCommand: %s %s %s\n",pcmd, arg1, arg2); fclose(fp); }

      if (FStrEq(pcmd, "addbot"))
      {
         BotCreate( pEntity, arg1, arg2, arg3, arg4 );

         return;
      }
      else if (FStrEq(pcmd, "debug_engine"))
      {
         debug_engine = 1;

         ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "debug_engine enabled!\n");

         return;
      }
      else if (FStrEq(pcmd, "waypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_waypoint_on = TRUE;

            ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "waypoints are ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_waypoint_on = FALSE;

            ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "waypoints are OFF\n");
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

            ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "waypoints saved\n");
         }
         else if (FStrEq(arg1, "load"))
         {
            if (WaypointLoad(pEntity))
               ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "waypoints loaded\n");
         }
         else if ((FStrEq(arg1, "stat")) || (FStrEq(arg1, "stats")))
         {
            WaypointPrintStat(pEntity);
         }
         else
         {
            if (g_waypoint_on)
               ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "waypoints are ON\n");
            else
               ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "waypoints are OFF\n");
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

         ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, msg);

         return;
      }
      else if (FStrEq(pcmd, "pathwaypoint"))
      {
         if (FStrEq(arg1, "on"))
         {
            g_path_waypoint = TRUE;
            g_waypoint_on = TRUE;  // turn this on just in case

            ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "pathwaypoint is ON\n");
         }
         else if (FStrEq(arg1, "off"))
         {
            g_path_waypoint = FALSE;

            ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "pathwaypoint is OFF\n");
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
      static float respawn_time = 0.0;
      static float previous_time = -1.0;
      static bool file_opened = FALSE;
      static int file_index, length;
      static FILE *bot_cfg_fp;
      static int ch;
      static char cmd_line[80];
      static char server_cmd[80];
      char *cmd, *arg1, *arg2, *arg3, *arg4;
      static float pause_time;
      char msg[256];

      for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
      {
         if (bots[bot_index].is_used)  // is this slot used?
         {
            BotThink(&bots[bot_index]);
         }
      }

      for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
      {
         pPlayer = INDEXENT(player_index);

         if (pPlayer && !pPlayer->free)
         {
            if ((g_waypoint_on) && FBitSet(pPlayer->v.flags, FL_CLIENT))
            {
                  WaypointThink(pPlayer);
            }
         }
      }

      // if a new map has started then...
      if (previous_time > gpGlobals->time)
      {
         // if haven't started respawning yet then set the respawn time
         if (respawn_time < 1.0)
            respawn_time = gpGlobals->time + 5.0;

         // set the time to check for automatically adding bots to dedicated servers
         bot_check_time = gpGlobals->time + 10.0;

         // if "map" command was used, set bots in use to respawn state...
         for (index = 0; index < 32; index++)
         {
            if ((bots[index].is_used) &&  // is this slot used?
                (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
            {
               // bot has already been "kicked" by server so just set flag
               bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
            }
         }
      }

      if (!IS_DEDICATED_SERVER())
      {
         if ((welcome_index != -1) && (welcome_sent == FALSE) &&
             (welcome_time < 1.0))
         {
            // are they out of observer mode yet?
            if (IsAlive(clients[welcome_index]))
               welcome_time = gpGlobals->time + 5.0;  // welcome in 5 seconds
         }

         if ((welcome_time > 0.0) && (welcome_time < gpGlobals->time) &&
             (welcome_sent == FALSE))
         {
            // let's send a welcome message to this client...
            UTIL_SayText(welcome_msg, clients[welcome_index]);

            welcome_sent = TRUE;  // clear this so we only do it once
         }
      }

      // are we currently respawning bots and is it time to spawn one yet?
      if ((respawn_time > 1.0) && (respawn_time <= gpGlobals->time))
      {
         int index = 0;

         // set the time to check for automatically adding bots to dedicated servers
         bot_check_time = gpGlobals->time + 5.0;

         // find bot needing to be respawned...
         while ((index < 32) &&
                (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
            index++;

         if (index < 32)
         {
            bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
            bots[index].is_used = FALSE;      // free up this slot

            // respawn 1 bot then wait a while (otherwise engine crashes)
            if ((mod_id == VALVE_DLL) || (mod_id == GEARBOX_DLL))
            {
               BotCreate(NULL, bots[index].skin, bots[index].name, NULL, NULL);
            }
            else
            {
               char c_team[2];
               char c_class[3];

               sprintf(c_team, "%d", bots[index].bot_team);
               sprintf(c_class, "%d", bots[index].bot_class);

               BotCreate(NULL, c_team, c_class, bots[index].name, NULL);
            }

            respawn_time = gpGlobals->time + 1.0;  // set next respawn time
         }
         else
         {
            respawn_time = 0.0;
         }
      }

      if (g_GameRules)
      {
         if (!file_opened)  // have we open bot.cfg file yet?
         {
            char bot_cfg_filename[256];
            int str_index;

            (*g_engfuncs.pfnGetGameDir)(bot_cfg_filename);

            strcat(bot_cfg_filename, "/bot.cfg");  // linux filename format

#ifndef __linux__
            str_index = 0;
            while (bot_cfg_filename[str_index])
            {
               if (bot_cfg_filename[str_index] == '/')
                  bot_cfg_filename[str_index] = '\\';  // MS-DOS format
               str_index++;
            }
#endif

            sprintf(msg, "Executing %s\n", bot_cfg_filename);
            ALERT( at_console, msg );

            bot_cfg_fp = fopen(bot_cfg_filename, "r");

            file_opened = TRUE;

            if (bot_cfg_fp == NULL)
               ALERT( at_console, "bot.cfg file not found\n" );

            pause_time = gpGlobals->time;

            file_index = 0;
            cmd_line[file_index] = 0;  // null out command line
         }

         // if the bot.cfg file is still open and time to execute command...
         while ((bot_cfg_fp != NULL) && !feof(bot_cfg_fp) &&
                (pause_time <= gpGlobals->time))
         {
            ch = fgetc(bot_cfg_fp);

            // skip any leading blanks
            while (ch == ' ')
               ch = fgetc(bot_cfg_fp);

            while ((ch != EOF) && (ch != '\r') && (ch != '\n'))
            {
               if (ch == '\t')  // convert tabs to spaces
                  ch = ' ';
      
               cmd_line[file_index] = ch;

               ch = fgetc(bot_cfg_fp);

               // skip multiple spaces in input file
               while ((cmd_line[file_index] == ' ') &&
                      (ch == ' '))      
                  ch = fgetc(bot_cfg_fp);
      
               file_index++;
            }

            if (ch == '\r')  // is it a carriage return?
            {
               ch = fgetc(bot_cfg_fp);  // skip the linefeed
            }
      
            cmd_line[file_index] = 0;  // terminate the command line
      
            // copy the command line to a server command buffer...
            strcpy(server_cmd, cmd_line);
            strcat(server_cmd, "\n");
      
            file_index = 0;
            cmd = cmd_line;
            arg1 = arg2 = arg3 = arg4 = NULL;
      
            // skip to blank or end of string...
            while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
               file_index++;
      
            if (cmd_line[file_index] == ' ')
            {
               cmd_line[file_index++] = 0;
               arg1 = &cmd_line[file_index];
      
               // skip to blank or end of string...
               while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
                  file_index++;
      
               if (cmd_line[file_index] == ' ')
               {
                  cmd_line[file_index++] = 0;
                  arg2 = &cmd_line[file_index];
      
                  // skip to blank or end of string...
                  while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
                     file_index++;
      
                  if (cmd_line[file_index] == ' ')
                  {
                     cmd_line[file_index++] = 0;
                     arg3 = &cmd_line[file_index];
      
                     // skip to blank or end of string...
                     while ((cmd_line[file_index] != ' ') && (cmd_line[file_index] != 0))
                        file_index++;
      
                     if (cmd_line[file_index] == ' ')
                     {
                        cmd_line[file_index++] = 0;
                        arg4 = &cmd_line[file_index];
                     }
                  }
               }
            }
      
            file_index = 0;  // reset for next input line
      
            if ((cmd_line[0] == '#') || (cmd_line[0] == 0))
            {
               continue;  // ignore comments or blank lines
            }
            else if (strcmp(cmd, "addbot") == 0)
            {
               BotCreate( NULL, arg1, arg2, arg3, arg4 );
      
               // have to delay here or engine gives "Tried to write to
               // uninitialized sizebuf_t" error and crashes...
      
               pause_time = gpGlobals->time + 1;
      
               break;
            }
            else if (strcmp(cmd, "min_bots") == 0)
            {
               min_bots = atoi( arg1 );

               if ((min_bots < 0) || (min_bots > 31))
                  min_bots = 1;
      
               if (IS_DEDICATED_SERVER())
               {
                  sprintf(msg, "min_bots set to %d\n", min_bots);
                  printf(msg);
               }
            }
            else if (strcmp(cmd, "max_bots") == 0)
            {
               max_bots = atoi( arg1 );
     
               if ((max_bots < 0) || (max_bots > 31)) 
                  max_bots = 1;

               if (IS_DEDICATED_SERVER())
               {
                  sprintf(msg, "max_bots set to %d\n", max_bots);
                  printf(msg);
               }
            }
            else if (strcmp(cmd, "pause") == 0)
            {
               pause_time = gpGlobals->time + atoi( arg1 );
               break;
            }
            else
            {
               sprintf(msg, "executing server command: %s\n", server_cmd);
               ALERT( at_console, msg );
      
               if (IS_DEDICATED_SERVER())
                  printf(msg);
      
               SERVER_COMMAND(server_cmd);
            }
         }
      
         // if bot.cfg file is open and reached end of file, then close and free it
         if ((bot_cfg_fp != NULL) && feof(bot_cfg_fp))
         {
            fclose(bot_cfg_fp);

            bot_cfg_fp = NULL;
         }
      }      

      // if time to check for server commands then do so...
      if ((check_server_cmd <= gpGlobals->time) && IS_DEDICATED_SERVER())
      {
         check_server_cmd = gpGlobals->time + 1.0;

         char *cvar_bot = (char *)CVAR_GET_STRING( "bot" );

         if ( cvar_bot && cvar_bot[0] )
         {
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

extern "C" EXPORT int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
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
         return "???";
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

