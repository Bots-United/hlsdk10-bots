/***
*
*  Copyright (c) 1999, Valve LLC. All rights reserved.
*
*  This product contains software technology licensed from Id 
*  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*  All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

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

$Source: /usr/local/cvsroot/csbot/src/dlls/util.cpp,v $
$Revision: 1.5 $
$Date: 2001/04/30 08:06:11 $
$State: Exp $

***********************************

$Log: util.cpp,v $
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
#include "util.h"
#include "engine.h"

#include "ai.h"
#include "bot.h"
#include "bot_func.h"


extern bot_t bots[MAX_BOTS];
extern char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
extern int num_teams;

int gmsgTextMsg = 0;
int gmsgSayText = 0;
int gmsgShowMenu = 0;


Vector UTIL_VecToAngles( const Vector &vec )
{
   float rgflVecOut[3];
   VEC_TO_ANGLES(vec, rgflVecOut);
   return Vector(rgflVecOut);
}


// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}


void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}


void UTIL_MakeVectors( const Vector &vecAngles )
{
   MAKE_VECTORS( vecAngles );
}


edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
   edict_t  *pentEntity;

   pentEntity = FIND_ENTITY_IN_SPHERE( pentStart, vecCenter, flRadius);

   if (!FNullEnt(pentEntity))
      return pentEntity;

   return NULL;
}


edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
   edict_t *pentEntity;

   pentEntity = FIND_ENTITY_BY_STRING( pentStart, szKeyword, szValue );

   if (!FNullEnt(pentEntity))
      return pentEntity;
   return NULL;
}

edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "classname", szName );
}

edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "targetname", szName );
}


int UTIL_PointContents( const Vector &vec )
{
   return POINT_CONTENTS(vec);
}


void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax )
{
   SET_SIZE( ENT(pev), vecMin, vecMax );
}


void UTIL_SetOrigin( entvars_t *pev, const Vector &vecOrigin )
{
   SET_ORIGIN(ENT(pev), vecOrigin );
}


void ClientPrint( edict_t *pEntity, int msg_dest, const char *msg_name)
{
   if (gmsgTextMsg == 0)
      gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

   pfnMessageBegin( MSG_ONE, gmsgTextMsg, NULL, pEntity );

   pfnWriteByte( msg_dest );
   pfnWriteString( msg_name );
   pfnMessageEnd();
}

void UTIL_SayText( const char *pText, edict_t *pEdict )
{
   if (gmsgSayText == 0)
      gmsgSayText = REG_USER_MSG( "SayText", -1 );

   pfnMessageBegin( MSG_ONE, gmsgSayText, NULL, pEdict );
      pfnWriteByte( ENTINDEX(pEdict) );
      pfnWriteString( pText );
   pfnMessageEnd();
}


void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message )
{
   int   j;
   char  text[128];
   char *pc;
   int   sender_team, player_team;
   edict_t *client;

   // make sure the text has content
   for ( pc = message; pc != NULL && *pc != 0; pc++ )
   {
      if ( isprint( *pc ) && !isspace( *pc ) )
      {
         pc = NULL;   // we've found an alphanumeric character,  so text is valid
         break;
      }
   }

   if ( pc != NULL )
      return;  // no character found, so say nothing

   // turn on color set 2  (color on,  no sound)
   if ( teamonly )
      sprintf( text, "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
   else
      sprintf( text, "%c%s: ", 2, STRING( pEntity->v.netname ) );

   j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
   if ( (int)strlen(message) > j )
      message[j] = 0;

   strcat( text, message );
   strcat( text, "\n" );

   // loop through all players
   // Start with the first player.
   // This may return the world in single player if the client types something between levels or during spawn
   // so check it, or it will infinite loop

   if (gmsgSayText == 0)
      gmsgSayText = REG_USER_MSG( "SayText", -1 );

   sender_team = UTIL_GetTeam(pEntity);

   client = NULL;
   while ( ((client = UTIL_FindEntityByClassname( client, "player" )) != NULL) &&
           (!FNullEnt(client)) ) 
   {
      if ( client == pEntity )  // skip sender of message
         continue;

      player_team = UTIL_GetTeam(client);

      if ( teamonly && (sender_team != player_team) )
         continue;

      pfnMessageBegin( MSG_ONE, gmsgSayText, NULL, client );
         pfnWriteByte( ENTINDEX(pEntity) );
         pfnWriteString( text );
      pfnMessageEnd();
   }

   // print to the sending client
   pfnMessageBegin( MSG_ONE, gmsgSayText, NULL, pEntity );
      pfnWriteByte( ENTINDEX(pEntity) );
      pfnWriteString( text );
   pfnMessageEnd();
   
   // echo to server console
   g_engfuncs.pfnServerPrint( text );
}


#ifdef   DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
   if (pev->pContainingEntity != NULL)
      return pev->pContainingEntity;
   ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine");
   edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
   if (pent == NULL)
      ALERT(at_console, "DAMN!  Even the engine couldn't FindEntityByVars!");
   ((entvars_t *)pev)->pContainingEntity = pent;
   return pent;
}
#endif //DEBUG


// return team number 0 through 3 based what MOD uses for team numbers
int UTIL_GetTeam(edict_t *pEntity)
{
      char *infobuffer;
      char model_name[32];

      infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEntity );
      strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

      if ((strcmp(model_name, "terror") == 0) ||  // Phoenix Connektion
          (strcmp(model_name, "arab") == 0) ||    // old L337 Krew
          (strcmp(model_name, "leet") == 0) ||    // L337 Krew
          (strcmp(model_name, "arctic") == 0) ||   // Arctic Avenger
          (strcmp(model_name, "guerilla") == 0))  // Gorilla Warfare
      {
         return 0;
      }
      else if ((strcmp(model_name, "urban") == 0) ||  // Seal Team 6
               (strcmp(model_name, "gsg9") == 0) ||   // German GSG-9
               (strcmp(model_name, "sas") == 0) ||    // UK SAS
               (strcmp(model_name, "gign") == 0) ||   // French GIGN
               (strcmp(model_name, "vip") == 0))      // VIP
      {
         return 1;
      }

      return -1;  // return zero if team is unknown
}


// return class number 0 through N
int UTIL_GetClass(edict_t *pEntity)
{
   char *infobuffer;
   char model_name[32];

   infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)( pEntity );
   strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

   return 0;
}


int UTIL_GetBotIndex(edict_t *pEdict)
{
   int index;

   for (index=0; index < MAX_BOTS; index++)
   {
      if (bots[index].pEdict == pEdict)
      {
         return index;
      }
   }

   return -1;  // return -1 if edict is not a bot
}


bot_t *UTIL_GetBotPointer(edict_t *pEdict)
{
   int index;

   for (index=0; index < MAX_BOTS; index++)
   {
      if (bots[index].pEdict == pEdict)
      {
         break;
      }
   }

   if (index < MAX_BOTS)
      return (&bots[index]);

   return NULL;  // return NULL if edict is not a bot
}


bool IsAlive(edict_t *pEdict)
{
   return ((pEdict->v.deadflag == DEAD_NO) &&
           (pEdict->v.health > 0) && !(pEdict->v.flags & FL_NOTARGET));
}


bool FInViewCone(Vector *pOrigin, edict_t *pEdict)
{
   Vector2D vec2LOS;
   float    flDot;

   UTIL_MakeVectors ( pEdict->v.angles );

   vec2LOS = ( *pOrigin - pEdict->v.origin ).Make2D();
   vec2LOS = vec2LOS.Normalize();

   flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

   if ( flDot > 0.50 )  // 60 degree field of view (90deg, .78rad)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


bool FVisible( const Vector &vecOrigin, edict_t *pEdict )
{
   TraceResult tr;
   Vector      vecLookerOrigin;

   vecLookerOrigin = eyePosition (pEdict);

   int bInWater = (UTIL_PointContents (vecOrigin) == CONTENTS_WATER);
   int bLookerInWater = (UTIL_PointContents (vecLookerOrigin) == CONTENTS_WATER);

   // don't look through water
   if (bInWater != bLookerInWater)
      return FALSE;

   //UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);
   UTIL_TraceLine( vecLookerOrigin, vecOrigin, ignore_monsters, pEdict->v.pContainingEntity, &tr );

   if (tr.flFraction != 1.0)
   {
      return FALSE;  // Line of sight is not established
   }
   else
   {
      return TRUE;  // line of sight is valid.
   }
}


Vector GetGunPosition(edict_t *pEdict)
{
   return (pEdict->v.origin + pEdict->v.view_ofs);
}


void UTIL_SelectItem(edict_t *pEdict, char *item_name)
{
   FakeClientCommand(pEdict, item_name, NULL, NULL);
}


Vector VecBModelOrigin(edict_t *pEdict)
{
   return pEdict->v.absmin + (pEdict->v.size * 0.5);
}


void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText )
{
   if (gmsgShowMenu == 0)
      gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );

   pfnMessageBegin( MSG_ONE, gmsgShowMenu, NULL, pEdict );

   pfnWriteShort( slots );
   pfnWriteChar( displaytime );
   pfnWriteByte( needmore );
   pfnWriteString( pText );

   pfnMessageEnd();
}

void UTIL_BuildFileName(char *filename, char *arg1, char *arg2)
{
#ifndef __linux__
   strcpy(filename, "cstrike\\");
#else
   strcpy(filename, "cstrike/");
#endif

   if ((arg1) && (*arg1) && (arg2) && (*arg2))
   {
      strcat(filename, arg1);

#ifndef __linux__
      strcat(filename, "\\");
#else
      strcat(filename, "/");
#endif

      strcat(filename, arg2);
   }
   else if ((arg1) && (*arg1))
   {
      strcat(filename, arg1);
   }
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( char *fmt, ... )
{
   va_list        argptr;
   static char    string[1024];
   
   va_start ( argptr, fmt );
   vsprintf ( string, fmt, argptr );
   va_end   ( argptr );

   // Print to server console
   ALERT( at_logged, "%s", string );
}

