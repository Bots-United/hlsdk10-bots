//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// waypoint.cpp
//

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "waypoint.h"


extern int mod_id;
extern int m_spriteTexture;

// waypoints with information bits (flags)
WAYPOINT waypoints[MAX_WAYPOINTS];

// number of waypoints currently in use
int num_waypoints;

// declare the array of head pointers to the path structures...
PATH *paths[MAX_WAYPOINTS];

// time that this waypoint was displayed (while editing)
float wp_display_time[MAX_WAYPOINTS];

bool g_waypoint_paths = FALSE;  // have any paths been allocated?
bool g_waypoint_on = FALSE;
bool g_auto_waypoint = FALSE;
bool g_path_waypoint = FALSE;
Vector last_waypoint;
float f_path_time = 0.0;


static FILE *fp;


void WaypointDebug(void)
{
   int y = 1, x = 1;

   fp=fopen("bot.txt","a");
   fprintf(fp,"WaypointDebug: LINKED LIST ERROR!!!\n");
   fclose(fp);

   x = x - 1;  // x is zero
   y = y / x;  // cause an divide by zero exception

   return;
}


// free the linked list of waypoint path nodes...
void WaypointFree(void)
{
   for (int i=0; i < MAX_WAYPOINTS; i++)
   {
      int count = 0;

      if (paths[i])
      {
         PATH *p = paths[i];
         PATH *p_next;

         while (p)  // free the linked list
         {
            p_next = p->next;  // save the link to next
            free(p);
            p = p_next;

#ifdef _DEBUG
            count++;
            if (count > 1000) WaypointDebug();
#endif
         }

         paths[i] = NULL;
      }
   }
}


// initialize the waypoint structures...
void WaypointInit(void)
{
   int i;

   // have any waypoint path nodes been allocated yet?
   if (g_waypoint_paths)
      WaypointFree();  // must free previously allocated path memory

   for (i=0; i < MAX_WAYPOINTS; i++)
   {
      waypoints[i].flags = 0;
      waypoints[i].origin = Vector(0,0,0);

      wp_display_time[i] = 0.0;

      paths[i] = NULL;  // no paths allocated yet
   }

   num_waypoints = 0;

   last_waypoint = Vector(0,0,0);
}


void WaypointAddPath(short int add_index, short int path_index)
{
   PATH *p, *prev;
   int i;
   int count = 0;

   p = paths[add_index];
   prev = NULL;

   // find an empty slot for new path_index...
   while (p != NULL)
   {
      i = 0;

      while (i < MAX_PATH_INDEX)
      {
         if (p->index[i] == -1)
         {
            p->index[i] = path_index;

            return;
         }

         i++;
      }

      prev = p;     // save the previous node in linked list
      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }

   p = (PATH *)malloc(sizeof(PATH));

   if (p == NULL)
   {
      // ERROR ALLOCATING MEMORY!!!
      return;
   }

   p->index[0] = path_index;
   p->index[1] = -1;
   p->index[2] = -1;
   p->index[3] = -1;
   p->next = NULL;

   if (prev != NULL)
      prev->next = p;  // link new node into existing list

   if (paths[add_index] == NULL)
      paths[add_index] = p;  // save head point if necessary
}


void WaypointDeletePath(short int del_index)
{
   PATH *p;
   int index, i;

   // search all paths for this index...
   for (index=0; index < num_waypoints; index++)
   {
      p = paths[index];

      int count = 0;

      // search linked list for del_index...
      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] == del_index)
            {
               p->index[i] = -1;  // unassign this path
            }

            i++;
         }

         p = p->next;  // go to next node in linked list

#ifdef _DEBUG
         count++;
         if (count > 100) WaypointDebug();
#endif
      }
   }
}


void WaypointDeletePath(short int path_index, short int del_index)
{
   PATH *p;
   int i;
   int count = 0;

   p = paths[path_index];

   // search linked list for del_index...
   while (p != NULL)
   {
      i = 0;

      while (i < MAX_PATH_INDEX)
      {
         if (p->index[i] == del_index)
         {
            p->index[i] = -1;  // unassign this path
         }

         i++;
      }

      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }
}


// find a path from the current waypoint. (pPath MUST be NULL on the
// initial call. subsequent calls will return other paths if they exist.)
int WaypointFindPath(PATH **pPath, int *path_index, int waypoint_index, int team)
{
   int index;
   int count = 0;

   if (*pPath == NULL)
   {
      *pPath = paths[waypoint_index];
      *path_index = 0;
   }

   if (*path_index == MAX_PATH_INDEX)
   {
      *path_index = 0;

      *pPath = (*pPath)->next;  // go to next node in linked list
   }

   while (*pPath != NULL)
   {
      while (*path_index < MAX_PATH_INDEX)
      {
         if ((*pPath)->index[*path_index] != -1)  // found a path?
         {
            // save the return value
            index = (*pPath)->index[*path_index];

            // skip this path if next waypoint is team specific and NOT this team
            if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
                ((waypoints[index].flags & W_FL_TEAM) != team))
            {
               (*path_index)++;
               continue;
            }

            // set up stuff for subsequent calls...
            (*path_index)++;

            return index;
         }

         (*path_index)++;
      }

      *path_index = 0;

      *pPath = (*pPath)->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug();
#endif
   }

   return -1;
}

// find the nearest waypoint to the player and return the index (-1 if not found)
int WaypointFindNearest(edict_t *pEntity, float range, int team)
{
   int i, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   min_index = -1;
   min_distance = 9999.0;

   for (i=0; i < num_waypoints; i++)
   {
      if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
         continue;  // skip any deleted waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[i].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[i].flags & W_FL_TEAM) != team))
         continue;

      distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if ((distance < min_distance) && (distance < range))
      {
         // if waypoint is visible from current position (even behind head)...
         UTIL_TraceLine( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
                         ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            min_index = i;
            min_distance = distance;
         }
      }
   }

   return min_index;
}


// find the nearest waypoint to the source postition and return the index
int WaypointFindNearest(Vector v_src, edict_t *pEntity, float range, int team)
{
   int index, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   if (num_waypoints < 1)
      return -1;

   // find the nearest waypoint...

   min_index = -1;
   min_distance = 9999.0;

   for (index=0; index < num_waypoints; index++)
   {
      if ((waypoints[index].flags & W_FL_DELETED) == W_FL_DELETED)
         continue;  // skip any deleted waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[index].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[index].flags & W_FL_TEAM) != team))
         continue;

      distance = (waypoints[index].origin - v_src).Length();

      if ((distance < min_distance) && (distance < range))
      {
         // if waypoint is visible from source position...
         UTIL_TraceLine( v_src, waypoints[index].origin, ignore_monsters,
                         pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            min_index = index;
            min_distance = distance;
         }
      }
   }

   return min_index;
}


void WaypointDrawBeam(edict_t *pEntity, Vector start, Vector end, int width,
        int noise, int red, int green, int blue, int brightness, int speed)
{
   MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
   WRITE_BYTE( TE_BEAMPOINTS);
   WRITE_COORD(start.x);
   WRITE_COORD(start.y);
   WRITE_COORD(start.z);
   WRITE_COORD(end.x);
   WRITE_COORD(end.y);
   WRITE_COORD(end.z);
   WRITE_SHORT( m_spriteTexture );
   WRITE_BYTE( 1 ); // framestart
   WRITE_BYTE( 10 ); // framerate
   WRITE_BYTE( 10 ); // life in 0.1's
   WRITE_BYTE( width ); // width
   WRITE_BYTE( noise );  // noise

   WRITE_BYTE( red );   // r, g, b
   WRITE_BYTE( green );   // r, g, b
   WRITE_BYTE( blue );   // r, g, b

   WRITE_BYTE( brightness );   // brightness
   WRITE_BYTE( speed );    // speed
   MESSAGE_END();
}


void WaypointAdd(edict_t *pEntity)
{
   int index;
   edict_t *pent = NULL;
   float radius = 40;
   char item_name[64];

   if (num_waypoints >= MAX_WAYPOINTS)
      return;

   index = 0;

   // find the next available slot for the new waypoint...
   while (index < num_waypoints)
   {
      if ((waypoints[index].flags & W_FL_DELETED) == W_FL_DELETED)
         break;

      index++;
   }

   waypoints[index].flags = 0;

   // store the origin (location) of this waypoint (use entity origin)
   waypoints[index].origin = pEntity->v.origin;

   // store the last used waypoint for the auto waypoint code...
   last_waypoint = pEntity->v.origin;

   // set the time that this waypoint was originally displayed...
   wp_display_time[index] = gpGlobals->time;


   Vector start, end;

   if ((pEntity->v.flags & FL_DUCKING) == FL_DUCKING)
   {
      waypoints[index].flags |= W_FL_CROUCH;  // crouching waypoint

      start = pEntity->v.origin - Vector(0, 0, 17);
      end = start + Vector(0, 0, 34);
   }
   else
   {
      start = pEntity->v.origin - Vector(0, 0, 34);
      end = start + Vector(0, 0, 68);
   }

   if (pEntity->v.movetype == MOVETYPE_FLY)
      waypoints[index].flags |= W_FL_LADDER;  // waypoint on a ladder


   //********************************************************
   // look for lift, ammo, flag, health, armor, etc.
   //********************************************************

   while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
   {
      strcpy(item_name, STRING(pent->v.classname));

      if (strcmp("item_healthkit", item_name) == 0)
      {
         ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "found a healthkit!\n");
         waypoints[index].flags = W_FL_HEALTH;
      }

      if (strncmp("item_armor", item_name, 10) == 0)
      {
         ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, "found some armor!\n");
         waypoints[index].flags = W_FL_ARMOR;
      }

      // *************
      // LOOK FOR AMMO
      // *************

   }

   // draw a blue waypoint
   WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

   EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0,
                   ATTN_NORM, 0, 100);

   // increment total number of waypoints if adding at end of array...
   if (index == num_waypoints)
      num_waypoints++;

   // calculate all the paths to this new waypoint
   for (int i=0; i < num_waypoints; i++)
   {
      if (i == index)
         continue;  // skip the waypoint that was just added

      // check if the waypoint is reachable from the new one (one-way)
      if ( WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity) )
      {
         WaypointAddPath(index, i);
      }

      // check if the new one is reachable from the waypoint (other way)
      if ( WaypointReachable(waypoints[i].origin, pEntity->v.origin, pEntity) )
      {
         WaypointAddPath(i, index);
      }
   }
}


void WaypointDelete(edict_t *pEntity)
{
   int index;
   int count = 0;

   if (num_waypoints < 1)
      return;

   index = WaypointFindNearest(pEntity, 50.0, -1);

   if (index == -1)
      return;

   // delete any paths that lead to this index...
   WaypointDeletePath(index);

   // free the path for this index...

   if (paths[index] != NULL)
   {
      PATH *p = paths[index];
      PATH *p_next;

      while (p)  // free the linked list
      {
         p_next = p->next;  // save the link to next
         free(p);
         p = p_next;

#ifdef _DEBUG
         count++;
         if (count > 100) WaypointDebug();
#endif
      }

      paths[index] = NULL;
   }

   waypoints[index].flags = W_FL_DELETED;  // not being used
   waypoints[index].origin = Vector(0,0,0);

   wp_display_time[index] = 0.0;

   EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/mine_activate.wav", 1.0,
                   ATTN_NORM, 0, 100);
}


// allow player to manually create a path from one waypoint to another
void WaypointCreatePath(edict_t *pEntity, int cmd)
{
   static int waypoint1 = -1;  // initialized to unassigned
   static int waypoint2 = -1;  // initialized to unassigned

   if (cmd == 1)  // assign source of path
   {
      waypoint1 = WaypointFindNearest(pEntity, 50.0, -1);

      if (waypoint1 == -1)
      {
         // play "cancelled" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      // play "start" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0,
                      ATTN_NORM, 0, 100);

      return;
   }

   if (cmd == 2)  // assign dest of path and make path
   {
      waypoint2 = WaypointFindNearest(pEntity, 50.0, -1);

      if ((waypoint1 == -1) || (waypoint2 == -1))
      {
         // play "error" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      WaypointAddPath(waypoint1, waypoint2);

      // play "done" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
                      ATTN_NORM, 0, 100);
   }
}


// allow player to manually remove a path from one waypoint to another
void WaypointRemovePath(edict_t *pEntity, int cmd)
{
   static int waypoint1 = -1;  // initialized to unassigned
   static int waypoint2 = -1;  // initialized to unassigned

   if (cmd == 1)  // assign source of path
   {
      waypoint1 = WaypointFindNearest(pEntity, 50.0, -1);

      if (waypoint1 == -1)
      {
         // play "cancelled" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      // play "start" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0,
                      ATTN_NORM, 0, 100);

      return;
   }

   if (cmd == 2)  // assign dest of path and make path
   {
      waypoint2 = WaypointFindNearest(pEntity, 50.0, -1);

      if ((waypoint1 == -1) || (waypoint2 == -1))
      {
         // play "error" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0,
                         ATTN_NORM, 0, 100);

         return;
      }

      WaypointDeletePath(waypoint1, waypoint2);

      // play "done" sound...
      EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0,
                      ATTN_NORM, 0, 100);
   }
}


bool WaypointLoad(edict_t *pEntity)
{
   char dirname[32];
   char filename[256];
   WAYPOINT_HDR header;
   char msg[80];
   int index;
   short int num;
   short int path_index;

   if (mod_id == TFC_DLL)
#ifndef __linux__
      strcpy(dirname, "tfc\\maps\\");
#else
      strcpy(dirname, "tfc/maps/");
#endif

   else if (mod_id == CSTRIKE_DLL)
#ifndef __linux__
      strcpy(dirname, "cstrike\\maps\\");
#else
      strcpy(dirname, "cstrike/maps/");
#endif

   else if (mod_id == GEARBOX_DLL)
#ifndef __linux__
      strcpy(dirname, "gearbox\\maps\\");
#else
      strcpy(dirname, "gearbox/maps/");
#endif

   else
#ifndef __linux__
      strcpy(dirname, "valve\\maps\\");
#else
      strcpy(dirname, "valve/maps/");
#endif

   strcpy(filename, dirname);
   strcat(filename, STRING(gpGlobals->mapname));
   strcat(filename, ".wpt");

#ifdef __linux__
   printf("loading waypoint file: %s\n", filename);
#endif

   FILE *bfp = fopen(filename, "rb");

   // if file exists, read the waypoint structure from it
   if (bfp != NULL)
   {
      fread(&header, sizeof(header), 1, bfp);

      header.filetype[7] = 0;
      if (strcmp(header.filetype, "HPB_bot") == 0)
      {
         if (header.waypoint_file_version != WAYPOINT_VERSION)
         {
            if (pEntity)
               ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, "Incompatible HPB bot waypoint file version!\nWaypoints not loaded!\n");

            fclose(bfp);
            return FALSE;
         }

         header.mapname[31] = 0;

         if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
         {
            WaypointInit();  // remove any existing waypoints

            for (int i=0; i < header.number_of_waypoints; i++)
            {
               fread(&waypoints[i], sizeof(waypoints[0]), 1, bfp);

               num_waypoints++;
            }

            // read and add waypoint paths...
            for (index=0; index < num_waypoints; index++)
            {
               // read the number of paths from this node...
               fread(&num, sizeof(num), 1, bfp);

               for (i=0; i < num; i++)
               {
                  fread(&path_index, sizeof(path_index), 1, bfp);

                  WaypointAddPath(index, path_index);
               }
            }

            g_waypoint_paths = TRUE;  // keep track so path can be freed
         }
         else
         {
            if (pEntity)
            {
               sprintf(msg, "%s HPB bot waypoints are not for this map!\n", filename);
               ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
            }

            fclose(bfp);
            return FALSE;
         }
      }
      else
      {
         if (pEntity)
         {
            sprintf(msg, "%s is not a HPB bot waypoint file!\n", filename);
            ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
         }

         fclose(bfp);
         return FALSE;
      }

      fclose(bfp);
   }
   else
   {
      if (pEntity)
      {
         sprintf(msg, "Waypoint file %s does not exist!\n", filename);
         ClientPrint(VARS(pEntity), HUD_PRINTNOTIFY, msg);
      }

#ifdef __linux__
      printf("waypoint file not found!\n");
#endif

      return FALSE;
   }

   return TRUE;
}


void WaypointSave(void)
{
   char dirname[32];
   char filename[256];
   WAYPOINT_HDR header;
   int index, i;
   short int num;
   PATH *p;

   strcpy(header.filetype, "HPB_bot");

   header.waypoint_file_version = WAYPOINT_VERSION;

   header.waypoint_file_flags = 0;  // not currently used

   header.number_of_waypoints = num_waypoints;

   memset(header.mapname, 0, sizeof(header.mapname));
   strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
   header.mapname[31] = 0;

   if (mod_id == TFC_DLL)
#ifndef __linux__
      strcpy(dirname, "tfc\\maps\\");
#else
      strcpy(dirname, "tfc/maps/");
#endif

   else if (mod_id == CSTRIKE_DLL)
#ifndef __linux__
      strcpy(dirname, "cstrike\\maps\\");
#else
      strcpy(dirname, "cstrike/maps/");
#endif

   else if (mod_id == GEARBOX_DLL)
#ifndef __linux__
      strcpy(dirname, "gearbox\\maps\\");
#else
      strcpy(dirname, "gearbox/maps/");
#endif

   else
#ifndef __linux__
      strcpy(dirname, "valve\\maps\\");
#else
      strcpy(dirname, "valve/maps/");
#endif

   strcpy(filename, dirname);
   strcat(filename, header.mapname);
   strcat(filename, ".wpt");

   FILE *bfp = fopen(filename, "wb");

   // write the waypoint header to the file...
   fwrite(&header, sizeof(header), 1, bfp);

   // write the waypoint data to the file...
   for (index=0; index < num_waypoints; index++)
   {
      fwrite(&waypoints[index], sizeof(waypoints[0]), 1, bfp);
   }

   // save the waypoint paths...
   for (index=0; index < num_waypoints; index++)
   {
      // count the number of paths from this node...

      p = paths[index];
      num = 0;

      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] != -1)
               num++;  // count path node if it's used

            i++;
         }

         p = p->next;  // go to next node in linked list
      }

      fwrite(&num, sizeof(num), 1, bfp);  // write the count

      // now write out each path index...

      p = paths[index];

      while (p != NULL)
      {
         i = 0;

         while (i < MAX_PATH_INDEX)
         {
            if (p->index[i] != -1)  // save path node if it's used
               fwrite(&p->index[i], sizeof(p->index[0]), 1, bfp);

            i++;
         }

         p = p->next;  // go to next node in linked list
      }
   }

   fclose(bfp);
}


bool WaypointReachable(Vector v_src, Vector v_dest, edict_t *pEntity)
{
   TraceResult tr;
   float curr_height, last_height;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < REACHABLE_RANGE)
   {
      // check if this waypoint is "visible"...

      UTIL_TraceLine( v_src, v_dest, ignore_monsters,
                      pEntity->v.pContainingEntity, &tr );

      // if waypoint is visible from current position (even behind head)...
      if (tr.flFraction >= 1.0)
      {
         // check for special case of both waypoints being underwater...
         if ((POINT_CONTENTS( v_src ) == CONTENTS_WATER) &&
             (POINT_CONTENTS( v_dest ) == CONTENTS_WATER))
         {
            return TRUE;
         }

         // check for special case of waypoint being suspended in mid-air...

         // is dest waypoint higher than src? (45 is max jump height)
         if (v_dest.z > (v_src.z + 45.0))
         {
            Vector v_new_src = v_dest;
            Vector v_new_dest = v_dest;

            v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units

            UTIL_TraceLine(v_new_src, v_new_dest, dont_ignore_monsters,
                           pEntity->v.pContainingEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
            {
               return FALSE;  // can't reach this one
            }
         }

         // check if distance to ground increases more than jump height
         // at points between source and destination...

         Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         Vector v_check = v_src;
         Vector v_down = v_src;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceLine(v_check, v_down, ignore_monsters,
                        pEntity->v.pContainingEntity, &tr);

         last_height = tr.flFraction * 1000.0;  // height from ground

         distance = (v_dest - v_check).Length();  // distance from goal

         while (distance > 10.0)
         {
            // move 10 units closer to the goal...
            v_check = v_check + (v_direction * 10.0);

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceLine(v_check, v_down, ignore_monsters,
                           pEntity->v.pContainingEntity, &tr);

            curr_height = tr.flFraction * 1000.0;  // height from ground

            // is the difference in the last height and the current height
            // higher that the jump height?
            if ((last_height - curr_height) > 45.0)
            {
               // can't get there from here...
               return FALSE;
            }

            last_height = curr_height;

            distance = (v_dest - v_check).Length();  // distance from goal
         }

         return TRUE;
      }
   }

   return FALSE;
}


// find the nearest reachable waypoint
int WaypointFindReachable(edict_t *pEntity, float range, int team)
{
   int i, min_index;
   float distance;
   float min_distance;
   TraceResult tr;

   // find the nearest waypoint...

   min_distance = 9999.0;

   for (i=0; i < num_waypoints; i++)
   {
      if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
         continue;  // skip any deleted waypoints

      // skip this waypoint if it's team specific and teams don't match...
      if ((team != -1) && (waypoints[i].flags & W_FL_TEAM_SPECIFIC) &&
          ((waypoints[i].flags & W_FL_TEAM) != team))
         continue;

      distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if (distance < min_distance)
      {
         // if waypoint is visible from current position (even behind head)...
         UTIL_TraceLine( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
                         ignore_monsters, pEntity->v.pContainingEntity, &tr );

         if (tr.flFraction >= 1.0)
         {
            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
            {
               min_index = i;
               min_distance = distance;
            }
         }
      }
   }

   // if not close enough to a waypoint then just return
   if (min_distance > range)
      return -1;

   return min_index;

}


void WaypointPrintStat(edict_t *pEntity)
{
   char msg[80];
   int index;
   int flags;

   // find the nearest waypoint...
   index = WaypointFindNearest(pEntity, 50.0, -1);

   if (index == -1)
      return;

   sprintf(msg,"Waypoint %d of %d total\n", index, num_waypoints);
   ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, msg);

   flags = waypoints[index].flags;

   if (flags & W_FL_TEAM_SPECIFIC)
   {
      if ((flags & W_FL_TEAM) == 0)
         strcpy(msg, "Waypoint is for TEAM 1\n");
      else if ((flags & W_FL_TEAM) == 1)
         strcpy(msg, "Waypoint is for TEAM 2\n");
      else if ((flags & W_FL_TEAM) == 2)
         strcpy(msg, "Waypoint is for TEAM 3\n");
      else if ((flags & W_FL_TEAM) == 3)
         strcpy(msg, "Waypoint is for TEAM 4\n");

      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, msg);
   }

   if (flags & W_FL_LIFT)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "Bot will wait for lift before approaching\n");

   if (flags & W_FL_LADDER)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "This waypoint is on a ladder\n");

   if (flags & W_FL_PAUSE)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "Bot will pause at this waypoint\n");

   if (flags & W_FL_HEALTH)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "There is health near this waypoint\n");

   if (flags & W_FL_ARMOR)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "There is armor near this waypoint\n");

   if (flags & W_FL_AMMO)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "There is ammo near this waypoint\n");

   if (flags & W_FL_SNIPER)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "This is a sniper waypoint\n");

   if (flags & W_FL_FLAG)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "There is a flag near this waypoint\n");

   if (flags & W_FL_FLAG_GOAL)
      ClientPrint( VARS(pEntity), HUD_PRINTNOTIFY, "There is a flag goal near this waypoint\n");
}


void WaypointThink(edict_t *pEntity)
{
   float distance, min_distance;
   Vector start, end;
   int i, index;

   if (g_auto_waypoint)  // is auto waypoint on?
   {
      // find the distance from the last used waypoint
      distance = (last_waypoint - pEntity->v.origin).Length();

      if (distance > 200)
      {
         min_distance = 9999.0;

         // check that no other reachable waypoints are nearby...
         for (i=0; i < num_waypoints; i++)
         {
            if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
               continue;

            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
            {
               distance = (waypoints[i].origin - pEntity->v.origin).Length();

               if (distance < min_distance)
                  min_distance = distance;
            }
         }

         // make sure nearest waypoint is far enough away...
         if (min_distance >= 200)
            WaypointAdd(pEntity);  // place a waypoint here
      }
   }

   min_distance = 9999.0;

   if (g_waypoint_on)  // display the waypoints if turned on...
   {
      for (i=0; i < num_waypoints; i++)
      {
         if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
            continue;

         distance = (waypoints[i].origin - pEntity->v.origin).Length();

         if (distance < 500)
         {
            if (distance < min_distance)
            {
               index = i; // store index of nearest waypoint
               min_distance = distance;
            }

            if ((wp_display_time[i] + 1.0) < gpGlobals->time)
            {
               if (waypoints[i].flags & W_FL_CROUCH)
               {
                  start = waypoints[i].origin - Vector(0, 0, 17);
                  end = start + Vector(0, 0, 34);
               }
               else
               {
                  start = waypoints[i].origin - Vector(0, 0, 34);
                  end = start + Vector(0, 0, 68);
               }

               // draw a blue waypoint
               WaypointDrawBeam(pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

               wp_display_time[i] = gpGlobals->time;
            }
         }
      }

      // check if path waypointing is on...
      if (g_path_waypoint)
      {
         // check if player is close enough to a waypoint and time to draw path...
         if ((min_distance <= 50) && (f_path_time <= gpGlobals->time))
         {
            PATH *p;

            f_path_time = gpGlobals->time + 1.0;

            p = paths[index];

            while (p != NULL)
            {
               i = 0;

               while (i < MAX_PATH_INDEX)
               {
                  if (p->index[i] != -1)
                  {
                     Vector v_src = waypoints[index].origin;
                     Vector v_dest = waypoints[p->index[i]].origin;

                     // draw a white line to this index's waypoint
                     WaypointDrawBeam(pEntity, v_src, v_dest, 10, 2, 250, 250, 250, 200, 10);
                  }

                  i++;
               }

               p = p->next;  // go to next node in linked list
            }
         }
      }
   }
}

