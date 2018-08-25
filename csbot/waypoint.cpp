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

$Source: /usr/local/cvsroot/csbot/src/dlls/waypoint.cpp,v $
$Revision: 1.11 $
$Date: 2001/05/11 00:49:06 $
$State: Exp $

***********************************

$Log: waypoint.cpp,v $
Revision 1.11  2001/05/11 00:49:06  eric
Fixed install to not clash with other HPB-based bots

Revision 1.10  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.9  2001/05/09 10:34:57  eric
botkillall command, NPE safeties

Revision 1.8  2001/05/06 01:48:37  eric
Replaced global constants with enums where appropriate

Revision 1.7  2001/05/04 08:58:14  eric
Windows install improvements

Revision 1.6  2001/04/30 10:44:20  eric
Tactical terrain evaluation fundamentals

Revision 1.5  2001/04/30 08:06:11  eric
willFire() deals with crouching versus standing

Revision 1.4  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.3  2001/04/29 01:45:36  eric
Tweaked CVS info headers

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef __linux__
#include <io.h>
#endif
#include <fcntl.h>
#ifndef __linux__
#include <sys\stat.h>
#else
#include <sys/stat.h>
#endif

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "ai.h"
#include "ai_terrain.h"
#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"

extern int m_spriteTexture;

WAYPOINT waypoints[MAX_WAYPOINTS];
int num_waypoints;
PATH *paths[MAX_WAYPOINTS]; // head pointers to path structures
VISIBILITY *visibility[MAX_WAYPOINTS]; // head pointers to visibility structures
TERRAIN terrain[MAX_WAYPOINTS]; // head pointers to terrain structures
float wp_display_time[MAX_WAYPOINTS]; // time that this waypoint was displayed (while editing)

bool g_waypoint_paths = false;  // have any paths been allocated?
bool g_waypoint_visibility = false; // have any visiblities been allocated?
bool g_waypoint_on = false;
bool g_auto_waypoint = false;
bool g_path_waypoint = false;
Vector last_waypoint;
float f_path_time = 0.0;

unsigned int route_num_waypoints;
unsigned short *shortest_path = NULL;
unsigned short *from_to = NULL;

static FILE *fp;


/** Prints waypoint debugging output. */
void WaypointDebug (char *str) {
   int y = 1, x = 1;

   fp=fopen ("bot.txt", "a");
   fprintf (fp, "WaypointDebug: LINKED LIST ERROR in %s\n", str);
   fclose (fp);

   x = x - 1;  // x is zero
   y = y / x;  // cause an divide by zero exception

   return;
}

/** Returns the number of paths from the given waypoint. */
short int numPaths (WP_INDEX wp) {
	PATH *p = paths[wp];
	short int num = 0;

	while (p != NULL) {
		int i = 0;

		while (i < MAX_PATH_INDEX) {
			if (p->index[i] != WP_INVALID)
				num++;

			i++;
		}

		p = p->next;
	}

	return num;
}

/**
 * Returns the number of visible
 * waypoints from the given waypoint.
 */
short int numVisibilities (WP_INDEX wp) {
	VISIBILITY *v = visibility[wp];
	short int num = 0;

	while (v != NULL) {
		num++;
		v = v->next;
	}

	return num;
}

/**
 * Returns TRUE iff there is a valid path
 * from one waypoint to the other. This
 * is an asymetrical function.
 */
bool isPath (int from, int to) {
	PATH *pPath = NULL;
	int wp_path = WP_INVALID;
	int pI;
	while ((wp_path = WaypointFindPath (&pPath, &pI, from)) != WP_INVALID) {
		if (wp_path == to)
			return true;
	}

	return false;
}

/** Frees the paths linked list of the given waypoint. */
void WaypointFreePath (int wp) {
	int count = 0;

	// free paths
	if (paths[wp] != NULL) {
		PATH *p = paths[wp];
		PATH *p_next;

		while (p) { // free the linked list
			p_next = p->next;
			free (p);
			p = p_next;

#ifdef _DEBUG
			count++;
			if (count > 1000) WaypointDebug ("WaypointFreePath");
#endif
		}

		paths[wp] = NULL;
	}
}


/** Frees the linked list for all waypoint path nodes. */
void WaypointFreePaths (void) {
	for (int i = 0; i < MAX_WAYPOINTS; i++) {
		WaypointFreePath (i);
	}
}

/** Frees the visibility linked list for the given waypoint. */
void WaypointFreeVisibility (int wp) {
	int count = 0;

	if (visibility[wp] != NULL) {
		VISIBILITY *v = visibility[wp];
		VISIBILITY *v_next;

		while (v) { // free the linked list
			v_next = v->next;
			free (v);
			v = v_next;

#ifdef _DEBUG
			count++;
			if (count > 1000) WaypointDebug ("WaypointFreeVisibility");
#endif
		}

		visibility[wp] = NULL;
	}
}

/** Frees the linked list of waypoint visiblity nodes. */
void WaypointFreeVisibilities (void) {
	for (int i = 0; i < MAX_WAYPOINTS; i++) {
		WaypointFreeVisibility (i);
	}
}

/** Initialize the waypoint structures. */
void WaypointInit (void) {
	if (g_waypoint_paths)
		WaypointFreePaths (); // must free previously allocated path memory

	if (g_waypoint_visibility)
		WaypointFreeVisibilities (); // must free previously allocated visibility memory

	if (shortest_path != NULL)
		free (shortest_path);

	if (from_to != NULL)
		free (from_to);

   for (int i = 0; i < MAX_WAYPOINTS; i++) {
      waypoints[i].flags = 0;
      waypoints[i].origin = Vector (0,0,0);

      wp_display_time[i] = 0.0;

	  terrainReset (&terrain[i]);

      paths[i] = NULL;  // no paths allocated yet
	  visibility[i] = NULL;  // no visibilities allocated yet
   }

   f_path_time = 0.0;  // reset waypoint path display time
   num_waypoints = 0;
   last_waypoint = Vector (0,0,0);

   shortest_path = NULL;
   from_to = NULL;
}

/**
 * Add a path from add_index
 * waypoint to path_index waypoint.
 */
void WaypointAddPath (WP_INDEX add_index, WP_INDEX path_index) {
   PATH *p, *prev;
   int i;
   int count = 0;

   p = paths[add_index];
   prev = NULL;

   // find an empty slot for new path_index...
   while (p != NULL) {
      i = 0;

      while (i < MAX_PATH_INDEX) {
         if (p->index[i] == WP_INVALID) {
            p->index[i] = path_index;
            return;
         }

         i++;
      }

      prev = p;     // save the previous node in linked list
      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug ("WaypointAddPath");
#endif
   }

   p = (PATH *)malloc (sizeof (PATH));

   if (p == NULL)
      ALERT (at_error, "TheBot - Error allocating memory for path!");

   p->index[0] = path_index;
   p->index[1] = WP_INVALID;
   p->index[2] = WP_INVALID;
   p->index[3] = WP_INVALID;
   p->next = NULL;

   if (prev != NULL)
      prev->next = p;  // link new node into existing list

   if (paths[add_index] == NULL)
      paths[add_index] = p;  // save head point if necessary
}

/** Add visibility link between src and visible. */
void WaypointAddVisibility (WP_INDEX src, WP_INDEX visible) {
	VISIBILITY *v = visibility[src];
	VISIBILITY *prev = NULL;
	VISIBILITY *prev2 = NULL;

	int count = 0;
	while (v != NULL) {
		if (v->index == visible)
			return;

		prev2 = prev;
		prev = v;
		v = v->next;

#ifdef _DEBUG
		count++;
		if (count > (100 * 4)) {
			char dbg[100];
			sprintf (dbg,
				"WaypointAddVisibility (src=%i) (visible=%i) (prev2=%i) (prev=%i) (v=%i)",
				src, visible, prev2 ? prev2->index : -4011, prev ? prev->index : -4011, v ? v->index : -4011);
			WaypointDebug (dbg);
		}
#endif
	}

	v = (VISIBILITY *)malloc (sizeof (VISIBILITY));

	if (v == NULL)
		ALERT (at_error, "TheBot - Error allocating memory for visibility!");

	v->index = visible;
	v->next = NULL;

	if (prev != NULL)
		prev->next = v; // link to end of existing list

	if (visibility[src] == NULL)
		visibility[src] = v; // save head point
}

/** Delete all paths to the given waypoint. */
void WaypointDeletePath (WP_INDEX del_index) {
   PATH *p;
   int index, i;

   // search all paths for this index...
   for (index = 0; index < num_waypoints; index++) {
      p = paths[index];

      int count = 0;

      // search linked list for del_index...
      while (p != NULL) {
         i = 0;

         while (i < MAX_PATH_INDEX) {
            if (p->index[i] == del_index) {
               p->index[i] = WP_INVALID;  // unassign this path
            }

            i++;
         }

         p = p->next;  // go to next node in linked list

#ifdef _DEBUG
         count++;
         if (count > 100) WaypointDebug ("WaypointDeletePath");
#endif
      }
   }
}

/**
 * Deletes a path from path_index
 * waypoint to del_index waypoint.
 */
void WaypointDeletePath (WP_INDEX path_index, WP_INDEX del_index) {
   PATH *p;
   int i;
   int count = 0;

   p = paths[path_index];

   // search linked list for del_index...
   while (p != NULL) {
      i = 0;

      while (i < MAX_PATH_INDEX) {
         if (p->index[i] == del_index) {
            p->index[i] = WP_INVALID;  // unassign this path
         }

         i++;
      }

      p = p->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug ("WaypointDeletePath");
#endif
   }
}

/** Delete all visibilites to the given waypoint. */
void WaypointDeleteVisibility (WP_INDEX del_index) {
	VISIBILITY *v, *v_prev;

	for (int i = 0; i < num_waypoints; i++) {
		v = visibility[i];
		v_prev = NULL;

		int count = 0;
		while (v != NULL) {
			VISIBILITY *v_next = v->next;

			if (v->index == del_index) {
				free (v);
				if (v_prev != NULL)
					v_prev->next = v_next;
				else
					visibility[i] = v_next;
				v_prev = v_prev; // since we just removed this link, the previous has not yet changed
			} else {
				v_prev = v;
			}

			v = v_next;

#ifdef _DEBUG
			count++;
			if (count > (100 * 4)) WaypointDebug ("WaypointDeleteVisibility");
#endif
		}
	}
}

/**
 * Find a path from the current waypoint.
 * pPath MUST be NULL on the initial call.
 * Subsequent calls return other paths if
 * they exist.
 */
int WaypointFindPath (PATH **pPath, int *path_index, int waypoint_index) {
   int index;
   int count = 0;

   if (*pPath == NULL) {
      *pPath = paths[waypoint_index];
      *path_index = 0;
   }

   if (*path_index == MAX_PATH_INDEX) {
      *path_index = 0;
      *pPath = (*pPath)->next;  // go to next node in linked list
   }

   while (*pPath != NULL) {
      while (*path_index < MAX_PATH_INDEX) {
		  if ((*pPath)->index[*path_index] != WP_INVALID) { // found a path?
            index = (*pPath)->index[*path_index]; // save return value
            (*path_index)++; // set up stuff for subsequent calls
            return index;
         }

         (*path_index)++;
      }

      *path_index = 0;

      *pPath = (*pPath)->next;  // go to next node in linked list

#ifdef _DEBUG
      count++;
      if (count > 100) WaypointDebug ("WaypointFindPath");
#endif
   }

   return WP_INVALID;
}

/**
 * Returns the waypoint nearest
 * the entity position.
 */
int WaypointFindNearest (edict_t *pEntity, float range) {
   if (num_waypoints < 1)
      return WP_INVALID;

   int min_index = WP_INVALID;
   float min_distance = 9999.0;

   for (int i = 0; i < num_waypoints; i++) {
      if (waypoints[i].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      float distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if (distance < min_distance && distance < range) {
		 if (! traceHit (eyePosition (pEntity), waypoints[i].origin, pEntity->v.pContainingEntity, true)) {
            min_index = i;
            min_distance = distance;
         }
      }
   }

   return min_index;
}

/**
 * Returns the waypoint nearest
 * the source position.
 */
int WaypointFindNearest(Vector v_src, edict_t *pEntity, float range)
{
   if (num_waypoints < 1)
      return WP_INVALID;

   int min_index = WP_INVALID;
   float min_distance = 9999.0;

   for (int index = 0; index < num_waypoints; index++) {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      float distance = (waypoints[index].origin - v_src).Length();

      if ((distance < min_distance) && (distance < range)) {
		 if (! traceHit (v_src, waypoints[index].origin, pEntity->v.pContainingEntity, true)) {
            min_index = index;
            min_distance = distance;
         }
      }
   }

   return min_index;
}

/**
 * Returns the goal nearest the entity
 * position, matching the given flags.
 */
int WaypointFindNearestGoal (edict_t *pEntity, int src, int flags) {
   if (num_waypoints < 1)
      return WP_INVALID;

   int min_index = WP_INVALID;
   float min_distance = 99999;

   for (int index = 0; index < num_waypoints; index++) {
      if (index == src)
         continue;  // skip the source waypoint

      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      float distance = WaypointDistanceFromTo(src, index);

      if (distance < min_distance) {
         min_index = index;
         min_distance = distance;
      }
   }

   return min_index;
}

/** Find any waypoint completely at random. */
int WaypointFindRandomGoal (edict_t *pEntity) {
	int index;
	int count = 0;

	if (num_waypoints < 1)
      return WP_INVALID;

	while (count < 50) {
		count++;

		index = RANDOM_LONG(1, num_waypoints) - 1;

		if (waypoints[index].flags & W_FL_DELETED)
			continue;  // skip any deleted waypoints

		return index;
	}

	return WP_INVALID;
}

/**
 * Find a random goal matching the "flags"
 * bits and return the index of that waypoint.
 * NOTE: randomness degrades if more than 50 waypoints
 * meet the flags criteria.
 */
int WaypointFindRandomGoal (edict_t *pEntity, int flags) {
   if (flags == 0)
	   return WaypointFindRandomGoal (pEntity);

   int index;
   int indexes[50];
   int count = 0;

   if (num_waypoints < 1)
      return WP_INVALID;

   for (index = 0; index < num_waypoints; index++) {
      if (waypoints[index].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      if ((waypoints[index].flags & flags) != flags)
         continue;  // skip this waypoint if the flags don't match

      if (count < 50) {
         indexes[count] = index;
         count++;
      } else {
		 indexes[RANDOM_LONG(1, 50) - 1] = index;
	  }
   }

   if (count == 0)
      return WP_INVALID;

   index = RANDOM_LONG (1, count) - 1;

   return indexes[index];
}

void WaypointDrawBeam (edict_t *pEntity, Vector start, Vector end, int width,
	int noise, int red, int green, int blue, int brightness, int speed)
{
   MESSAGE_BEGIN (MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
   WRITE_BYTE (TE_BEAMPOINTS);
   WRITE_COORD (start.x);
   WRITE_COORD (start.y);
   WRITE_COORD (start.z);
   WRITE_COORD (end.x);
   WRITE_COORD (end.y);
   WRITE_COORD (end.z);
   WRITE_SHORT (m_spriteTexture);
   WRITE_BYTE (1); // framestart
   WRITE_BYTE (10); // framerate
   WRITE_BYTE (10); // life in 0.1's
   WRITE_BYTE (width); // width
   WRITE_BYTE (noise);  // noise

   WRITE_BYTE (red);   // r, g, b
   WRITE_BYTE (green);   // r, g, b
   WRITE_BYTE (blue);   // r, g, b

   WRITE_BYTE (brightness);
   WRITE_BYTE (speed);
   MESSAGE_END ();
}

void WaypointAdd (edict_t *pEntity) {
   edict_t *pent = NULL;
   float radius = 40;

   if (num_waypoints >= MAX_WAYPOINTS)
      return;

   int index = 0;

   // find the next available slot for the new waypoint...
   while (index < num_waypoints) {
      if (waypoints[index].flags & W_FL_DELETED)
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


   Vector start = pEntity->v.origin - Vector(0, 0, 34);
   Vector end = start + Vector(0, 0, 68);

   if ((pEntity->v.flags & FL_DUCKING) == FL_DUCKING) {
      waypoints[index].flags |= W_FL_CROUCH;  // crouching waypoint

      start = pEntity->v.origin - Vector (0, 0, 17);
      end = start + Vector(0, 0, 34);
   }

   if (pEntity->v.movetype == MOVETYPE_FLY)
      waypoints[index].flags |= W_FL_LADDER;  // waypoint on a ladder

   // draw a blue waypoint
   WaypointDrawBeam (pEntity, start, end, 30, 0, 0, 0, 255, 250, 5);

   EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0, ATTN_NORM, 0, 100);

   // increment total number of waypoints if adding at end of array...
   if (index == num_waypoints)
      num_waypoints++;

   // calculate all the paths and visibilities from this new waypoint
   for (int i=0; i < num_waypoints; i++) {
      if (i == index)
         continue;  // skip the waypoint that was just added (prevents circular linked lists)

      if (WaypointReachable (pEntity->v.origin, waypoints[i].origin, pEntity))
         WaypointAddPath (index, i);

      if (WaypointReachable (waypoints[i].origin, pEntity->v.origin, pEntity))
         WaypointAddPath (i, index);

	  if (! traceHit (waypoints[index].origin, waypoints[i].origin, pEntity->v.pContainingEntity, true))
		  WaypointAddVisibility (index, i);

	  if (! traceHit (waypoints[i].origin, waypoints[index].origin, pEntity->v.pContainingEntity, true))
		  WaypointAddVisibility (i, index);
   }

   // initialize terrain AFTER visiblity/pathing
   //terrainInit (&terrain[index], index);
}

void WaypointDelete (edict_t *pEntity) {
   int count = 0;

   if (num_waypoints < 1)
      return;

   int index = WaypointFindNearest (pEntity, 50.0);
   if (index == WP_INVALID)
      return;

   terrainReset (&terrain[index]);

   WaypointDeletePath (index);
   WaypointFreePath (index);

   WaypointDeleteVisibility (index);
   WaypointFreeVisibility (index);

   waypoints[index].flags = W_FL_DELETED;  // not being used
   waypoints[index].origin = Vector (0,0,0);

   wp_display_time[index] = 0.0;

   EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "weapons/mine_activate.wav", 1.0, ATTN_NORM, 0, 100);
}

/** Allow player to manually create a path from one waypoint to another. */
void WaypointCreatePath (edict_t *pEntity, int cmd) {
   static int waypoint1 = WP_INVALID;  // initialized to unassigned
   static int waypoint2 = WP_INVALID;  // initialized to unassigned

   if (cmd == 1) { // assign source of path
      waypoint1 = WaypointFindNearest (pEntity, 50.0);

      if (waypoint1 == WP_INVALID) {
         // play "cancelled" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0, ATTN_NORM, 0, 100);
         return;
      }

      // play "start" sound...
      EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0, ATTN_NORM, 0, 100);
      return;
   }

   if (cmd == 2) { // assign dest of path and make path
      waypoint2 = WaypointFindNearest (pEntity, 50.0);

      if ((waypoint1 == WP_INVALID) || (waypoint2 == WP_INVALID)) {
         // play "error" sound...
         EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0, ATTN_NORM, 0, 100);
         return;
      }

      WaypointAddPath (waypoint1, waypoint2);

      // play "done" sound...
      EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
   }
}

/** Allow player to manually remove a path from one waypoint to another. */
void WaypointRemovePath (edict_t *pEntity, int cmd) {
   static int waypoint1 = WP_INVALID;  // initialized to unassigned
   static int waypoint2 = WP_INVALID;  // initialized to unassigned

   if (cmd == 1) { // assign source of path
      waypoint1 = WaypointFindNearest(pEntity, 50.0);

      if (waypoint1 == WP_INVALID) {
         // play "cancelled" sound...
         EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "common/wpn_moveselect.wav", 1.0, ATTN_NORM, 0, 100);
         return;
      }

      // play "start" sound...
      EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "common/wpn_hudoff.wav", 1.0, ATTN_NORM, 0, 100);
      return;
   }

   if (cmd == 2) { // assign dest of path and make path
      waypoint2 = WaypointFindNearest (pEntity, 50.0);

      if ((waypoint1 == WP_INVALID) || (waypoint2 == WP_INVALID)) {
         // play "error" sound...
         EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "common/wpn_denyselect.wav", 1.0, ATTN_NORM, 0, 100);
         return;
      }

      WaypointDeletePath (waypoint1, waypoint2);

      // play "done" sound...
      EMIT_SOUND_DYN2 (pEntity, CHAN_WEAPON, "common/wpn_hudon.wav", 1.0, ATTN_NORM, 0, 100);
   }
}

/** Loads waypoints from disk. */
bool WaypointLoad (edict_t *pEntity) {
   char mapname[64];
   char filename[256];
   WAYPOINT_HDR header;
   char msg[80];

   strcpy (mapname, STRING (gpGlobals->mapname));
   strcat (mapname, ".csw");

   UTIL_BuildFileName (filename, "maps", mapname);

   if (IS_DEDICATED_SERVER ())
      printf ("loading waypoint file: %s\n", filename);

   FILE *bfp = fopen (filename, "rb");

   // if file exists, read the waypoint structure from it
   if (bfp != NULL) {
      fread (&header, sizeof (header), 1, bfp);

      header.filetype[7] = 0;
      if (strcmp (header.filetype, WAYPOINT_FILETYPE) == 0) {
         if (header.waypoint_file_version != WAYPOINT_VERSION) {
            if (pEntity)
               ClientPrint (pEntity, HUD_PRINTNOTIFY, "Incompatible TheBot waypoint file version!\nWaypoints not loaded!\n");

            fclose (bfp);
            return false;
         }

         header.mapname[31] = 0;

         if (strcmp (header.mapname, STRING (gpGlobals->mapname)) == 0) {
            WaypointInit ();  // remove any existing waypoints

            for (int n = 0; n < header.number_of_waypoints; n++) {
               fread (&waypoints[n], sizeof (waypoints[0]), 1, bfp);
               num_waypoints++;
            }

            // read waypoint paths
            for (int k = 0; k < num_waypoints; k++) {
			   short int num;
			   short int path_index;

               fread (&num, sizeof (num), 1, bfp); // read the number of paths from this node

               for (int i = 0; i < num; i++) {
                  fread (&path_index, sizeof (path_index), 1, bfp);
                  WaypointAddPath (k, path_index);
               }
            }

            g_waypoint_paths = true;

            // read waypoint visibility
            for (int j = 0; j < num_waypoints; j++) {
			   short int num;
			   short int vis_index;

               fread (&num, sizeof (num), 1, bfp); // read the number of visibilities from this node

               for (int i = 0; i < num; i++) {
                  fread (&vis_index, sizeof (vis_index), 1, bfp);
                  WaypointAddVisibility (j, vis_index);
               }
            }

			g_waypoint_visibility = true;
         } else {
            if (pEntity) {
               sprintf (msg, "%s TheBot waypoints are not for this map!\n", filename);
               ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);
            }

            fclose (bfp);
            return false;
         }
      } else {
         if (pEntity) {
            sprintf (msg, "%s is not a TheBot waypoint file!\n", filename);
            ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);
         }

         fclose (bfp);
         return false;
      }

      fclose (bfp);
      WaypointRouteInit ();
	  TerrainInitAll ();
   } else {
      if (pEntity) {
         sprintf (msg, "Waypoint file %s does not exist!\n", filename);
         ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);
      }

      if (IS_DEDICATED_SERVER ())
         printf ("waypoint file %s not found!\n", filename);

      return false;
   }

   return true;
}

/** Saves the current set of waypoints to disk. */
void WaypointSave (void) {
   char filename[256];
   char mapname[64];
   WAYPOINT_HDR header;

   strcpy (header.filetype, WAYPOINT_FILETYPE);
   header.waypoint_file_version = WAYPOINT_VERSION;
   header.number_of_waypoints = num_waypoints;

   memset (header.mapname, 0, sizeof (header.mapname));
   strncpy (header.mapname, STRING (gpGlobals->mapname), 31);
   header.mapname[31] = 0;

   strcpy (mapname, STRING (gpGlobals->mapname));
   strcat (mapname, ".csw");

   UTIL_BuildFileName (filename, "maps", mapname);

   FILE *bfp = fopen (filename, "wb");

   // write header to file
   fwrite (&header, sizeof (header), 1, bfp);

   // write waypoints to file
   for (int n = 0; n < num_waypoints; n++) {
      fwrite (&waypoints[n], sizeof (waypoints[0]), 1, bfp);
   }

   // write paths to file
   for (int k = 0; k < num_waypoints; k++) {
      short int num = numPaths (k);

      fwrite (&num, sizeof (num), 1, bfp);  // write the count

      PATH *p = paths[k];

      while (p != NULL) {
         int i = 0;
         while (i < MAX_PATH_INDEX) {
            if (p->index[i] != WP_INVALID)  // save path node if it's used
               fwrite (&p->index[i], sizeof (p->index[0]), 1, bfp);

            i++;
         }

         p = p->next;  // go to next node in linked list
      }
   }

	// write visibility to file
	for (int j = 0; j < num_waypoints; j++) {
		short int num = numVisibilities (j);

		fwrite (&num, sizeof (num), 1, bfp);  // write the count

		VISIBILITY *v = visibility[j];

		while (v != NULL) {
			fwrite (&v->index, sizeof (v->index), 1, bfp);
			v = v->next;
		}
	}

	fclose (bfp);
}

bool WaypointReachable (Vector v_src, Vector v_dest, edict_t *pEntity) {
   TraceResult tr;
   float curr_height, last_height;
   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < REACHABLE_RANGE) {
      // check if this waypoint is "visible"...

      UTIL_TraceLine (v_src, v_dest, ignore_monsters, pEntity->v.pContainingEntity, &tr);

      // if waypoint is visible from current position (even behind head)...
      if (tr.flFraction >= 1.0) {
         // check for special case of both waypoints being underwater...
         if ((POINT_CONTENTS (v_src) == CONTENTS_WATER) &&
             (POINT_CONTENTS (v_dest) == CONTENTS_WATER))
         {
            return true;
         }

         // check for special case of waypoint being suspended in mid-air...

         // is dest waypoint higher than src? (45 is max jump height)
         if (v_dest.z > (v_src.z + 45.0)) {
            Vector v_new_src = v_dest;
            Vector v_new_dest = v_dest;

            v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units

            UTIL_TraceLine (v_new_src, v_new_dest, dont_ignore_monsters, pEntity->v.pContainingEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
            {
               return false;  // can't reach this one
            }
         }

         // check if distance to ground increases more than jump height
         // at points between source and destination...

         Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         Vector v_check = v_src;
         Vector v_down = v_src;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceLine (v_check, v_down, ignore_monsters, pEntity->v.pContainingEntity, &tr);

         last_height = tr.flFraction * 1000.0;  // height from ground

         distance = (v_dest - v_check).Length();  // distance from goal

         while (distance > 10.0) {
            // move 10 units closer to the goal...
            v_check = v_check + (v_direction * 10.0);

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceLine (v_check, v_down, ignore_monsters, pEntity->v.pContainingEntity, &tr);

            curr_height = tr.flFraction * 1000.0;  // height from ground

            // is the difference in the last height and the current height
            // higher that the jump height?
            if ((last_height - curr_height) > 45.0)
            {
               // can't get there from here...
               return false;
            }

            last_height = curr_height;

            distance = (v_dest - v_check).Length();  // distance from goal
         }

         return true;
      }
   }

   return false;
}


/** Returns the nearest reachable waypoint. */
int WaypointFindReachable (edict_t *pEntity, float range) {
   int min_index = WP_INVALID;
   float min_distance = 9999.0;

   for (int i = 0; i < num_waypoints; i++) {
      if (waypoints[i].flags & W_FL_DELETED)
         continue;  // skip any deleted waypoints

      float distance = (waypoints[i].origin - pEntity->v.origin).Length();

      if (distance < min_distance) {
		 if (! traceHit (eyePosition (pEntity), waypoints[i].origin, pEntity->v.pContainingEntity, true)) {
            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity)) {
               min_index = i;
               min_distance = distance;
            }
         }
      }
   }

   // if not close enough to a waypoint then just return
   if (min_distance > range)
      return WP_INVALID;

   return min_index;
}

void WaypointPrintInfo (edict_t *pEntity) {
   char msg[80];

   int index = WaypointFindNearest (pEntity, 50.0);
   if (index == WP_INVALID)
      return;

   sprintf (msg,"Waypoint %d of %d total\n", index, num_waypoints);
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Paths leaving: %d\n", numPaths (index));
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   sprintf (msg,"Waypoint visible: %d\n", numVisibilities (index));
   ClientPrint (pEntity, HUD_PRINTNOTIFY, msg);

   int flags = waypoints[index].flags;

   if (flags & W_FL_LIFT)
      ClientPrint (pEntity, HUD_PRINTNOTIFY, "Bot will wait for lift before approaching\n");

   if (flags & W_FL_LADDER)
      ClientPrint (pEntity, HUD_PRINTNOTIFY, "This waypoint is on a ladder\n");

   if (flags & W_FL_DOOR)
      ClientPrint (pEntity, HUD_PRINTNOTIFY, "This is a door waypoint\n");

   if (flags & W_FL_BOMB_SITE)
      ClientPrint (pEntity, HUD_PRINTNOTIFY, "There is a bomb site at this waypoint\n");

   if (flags & W_FL_RESCUE_ZONE)
      ClientPrint (pEntity, HUD_PRINTNOTIFY, "There is a rescue point at this waypoint\n");

   WaypointPrintTerrainInfo (pEntity, index);
}

void WaypointThink (edict_t *pEntity) {
   float distance, min_distance;
   Vector start, end;
   int i, index;

   if (g_auto_waypoint) { // is auto waypoint on?
      // find the distance from the last used waypoint
      distance = (last_waypoint - pEntity->v.origin).Length();

      if (distance > WAYPOINT_SPACING) {
         min_distance = 9999.0;

         // check that no other reachable waypoints are nearby...
         for (i=0; i < num_waypoints; i++) {
            if (waypoints[i].flags & W_FL_DELETED)
               continue;

            if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity)) {
               distance = (waypoints[i].origin - pEntity->v.origin).Length();

               if (distance < min_distance)
                  min_distance = distance;
            }
         }

         // make sure nearest waypoint is far enough away...
         if (min_distance >= WAYPOINT_SPACING)
            WaypointAdd(pEntity);  // place a waypoint here
      }
   }

   min_distance = 9999.0;

   if (g_waypoint_on) { // display the waypoints if turned on...
      for (i=0; i < num_waypoints; i++) {
         if ((waypoints[i].flags & W_FL_DELETED) == W_FL_DELETED)
            continue;

         distance = (waypoints[i].origin - pEntity->v.origin).Length();

         if (distance < 500) {
            if (distance < min_distance) {
               index = i; // store index of nearest waypoint
               min_distance = distance;
            }

            if ((wp_display_time[i] + 1.0) < gpGlobals->time) {
               if (waypoints[i].flags & W_FL_CROUCH) {
                  start = waypoints[i].origin - Vector(0, 0, 17);
                  end = start + Vector(0, 0, 34);
               } else {
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
      if (g_path_waypoint) {
         // check if player is close enough to a waypoint and time to draw path...
         if ((min_distance <= 50) && (f_path_time <= gpGlobals->time)) {
            PATH *p;

            f_path_time = gpGlobals->time + 1.0;

            p = paths[index];

            while (p != NULL) {
               i = 0;

               while (i < MAX_PATH_INDEX) {
                  if (p->index[i] != WP_INVALID) {
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

/**
 * Generate the least cost path matrix
 * from the waypoint list.
 */
void WaypointFloyds (unsigned short *shortest_path, unsigned short *from_to) {
   unsigned int x, y, z;
   int changed = 1;
   int distance;

   for (y=0; y < route_num_waypoints; y++) {
      for (z=0; z < route_num_waypoints; z++) {
         from_to[y * route_num_waypoints + z] = z;
      }
   }

   while (changed) {
      changed = 0;

      for (x=0; x < route_num_waypoints; x++) {
         for (y=0; y < route_num_waypoints; y++) {
            for (z=0; z < route_num_waypoints; z++) {
               if ((shortest_path[y * route_num_waypoints + x] == WAYPOINT_UNREACHABLE) ||
                   (shortest_path[x * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
			   {
                  continue;
			   }

               distance = shortest_path[y * route_num_waypoints + x] +
                          shortest_path[x * route_num_waypoints + z];

               if (distance > WAYPOINT_MAX_DISTANCE)
                  distance = WAYPOINT_MAX_DISTANCE;

               if ((distance < shortest_path[y * route_num_waypoints + z]) ||
                   (shortest_path[y * route_num_waypoints + z] == WAYPOINT_UNREACHABLE))
               {
                  shortest_path[y * route_num_waypoints + z] = distance;
                  from_to[y * route_num_waypoints + z] = from_to[y * route_num_waypoints + x];
                  changed = 1;
               }
            }
         }
      }
   }
}

/**
 * Load the waypoint route files. Generate
 * these files if they do not yet exist.
 */
void WaypointRouteInit (void) {
   unsigned int index;
   unsigned int array_size;
   unsigned int row;
   int i, offset;
   unsigned int a, b;
   float distance;
   unsigned short *pShortestPath, *pFromTo;
   char msg[80];

   if (num_waypoints == 0)
      return;

   // save number of current waypoints in case waypoints get added later
   route_num_waypoints = num_waypoints;

   array_size = route_num_waypoints * route_num_waypoints;

   if (shortest_path == NULL) {
      shortest_path = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

   if (shortest_path == NULL)
      ALERT(at_error, "TheBot - Error allocating memory for shortest path!");

   from_to = (unsigned short *)malloc(sizeof(unsigned short) * array_size);

   if (from_to == NULL)
      ALERT(at_error, "TheBot - Error allocating memory for from to matrix!");

   pShortestPath = shortest_path;
   pFromTo = from_to;

   for (index=0; index < array_size; index++)
      pShortestPath[index] = WAYPOINT_UNREACHABLE;

   for (index=0; index < route_num_waypoints; index++)
      pShortestPath[index * route_num_waypoints + index] = 0;  // zero diagonal

   for (row=0; row < route_num_waypoints; row++) {
      if (paths[row] != NULL) {
         PATH *p = paths[row];

         while (p) {
            i = 0;

            while (i < MAX_PATH_INDEX) {
               if (p->index[i] != WP_INVALID) {
                  index = p->index[i];

                     distance = (waypoints[row].origin - waypoints[index].origin).Length();

                     if (distance > (float)WAYPOINT_MAX_DISTANCE)
                        distance = (float)WAYPOINT_MAX_DISTANCE;

                     if (distance > REACHABLE_RANGE) {
                        sprintf(msg, "Waypoint path distance > %4.1f at from %d to %d\n",
                                     REACHABLE_RANGE, row, index);
                        ALERT(at_console, msg);
                     } else {
                        offset = row * route_num_waypoints + index;

                        pShortestPath[offset] = (unsigned short)distance;
                     }
               }

               i++;
            }

            p = p->next;  // go to next node in linked list
         }
      }
   }

   // run Floyd's Algorithm to generate the from_to matrix...
   WaypointFloyds(pShortestPath, pFromTo);

   for (a=0; a < route_num_waypoints; a++) {
      for (b=0; b < route_num_waypoints; b++)
         if (pShortestPath[a * route_num_waypoints + b] == WAYPOINT_UNREACHABLE)
           pFromTo[a * route_num_waypoints + b] = WAYPOINT_UNREACHABLE;
      }
   }
}

/**
 * Returns the next waypoint index for the Floyd
 * matrix path when travelling from the given source
 * to the given destination.
 */
int WaypointRouteFromTo(int src, int dest) {
	if (from_to == NULL || (! validWaypoint (src)) || (! validWaypoint (dest)))
		return WP_INVALID;

	return from_to[src * route_num_waypoints + dest];
}

/**
 * Returns the total distance (based on the
 * Floyd matrix) of a path from the source waypoint
 * to the destination waypoint.
 */
int WaypointDistanceFromTo (int src, int dest) {
	if (shortest_path == NULL || (! validWaypoint (src)) || (! validWaypoint (dest)))
		return 99999 + 1; // WaypointFindNearestGoal.min_distance + 1

	return (int)(shortest_path[src * route_num_waypoints + dest]);
}
