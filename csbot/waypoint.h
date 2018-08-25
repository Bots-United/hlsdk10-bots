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

$Source: /usr/local/cvsroot/csbot/src/dlls/waypoint.h,v $
$Revision: 1.7 $
$Date: 2001/05/11 00:49:06 $
$State: Exp $

***********************************

$Log: waypoint.h,v $
Revision 1.7  2001/05/11 00:49:06  eric
Fixed install to not clash with other HPB-based bots

Revision 1.6  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.5  2001/04/30 10:44:20  eric
Tactical terrain evaluation fundamentals

Revision 1.4  2001/04/30 08:06:11  eric
willFire() deals with crouching versus standing

Revision 1.3  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <limits.h>

const int MAX_WAYPOINTS			= 1024;
const float WAYPOINT_SPACING	= 128.0;
const float REACHABLE_RANGE		= 256.0;

// waypoint flags (32 bits)
#define W_FL_CROUCH			(1<<1)
#define W_FL_JUMP			(1<<2)
#define W_FL_LADDER			(1<<3)
#define W_FL_LIFT			(1<<4)
#define W_FL_DOOR			(1<<5)
#define W_FL_BOMB_SITE		(1<<11)
#define W_FL_RESCUE_ZONE	(1<<12)
#define W_FL_DELETED		(1<<31)

const char WAYPOINT_FILETYPE[] = "the_bot";
const int WAYPOINT_VERSION = 7;

// waypoint file header
typedef struct {
	char filetype[8];  // should be "the_bot\0"
	int  waypoint_file_version;
	int  number_of_waypoints;
	char mapname[32];  // name of map for these waypoints
} WAYPOINT_HDR;

typedef struct {
	int    flags;
	Vector origin;
} WAYPOINT;

#define WP_INDEX short int
#define WAYPOINT_UNREACHABLE USHRT_MAX
#define WAYPOINT_MAX_DISTANCE (USHRT_MAX-1)
const int MAX_PATH_INDEX = 4;
const int WP_INVALID = -1;

typedef struct path {
	WP_INDEX index[MAX_PATH_INDEX]; // -1 means not used
	struct path *next;
} PATH;

/** Linked list of visible nodes. */
typedef struct visibility {
	WP_INDEX index;
	visibility *next;
} VISIBILITY;


bool isPath (int from, int to);
void WaypointInit (void);
int WaypointFindPath (PATH **pPath, int *path_index, int waypoint_index);
int WaypointFindNearest (edict_t *pEntity, float distance);
int WaypointFindNearest (Vector v_src, edict_t *pEntity, float range);
int WaypointFindNearestGoal (edict_t *pEntity, int src, int flags);
int WaypointFindRandomGoal (edict_t *pEntity);
int WaypointFindRandomGoal (edict_t *pEntity, int flags);
void WaypointAdd (edict_t *pEntity);
void WaypointDelete (edict_t *pEntity);
void WaypointCreatePath (edict_t *pEntity, int cmd);
void WaypointRemovePath (edict_t *pEntity, int cmd);
bool WaypointLoad (edict_t *pEntity);
void WaypointSave (void);
bool WaypointReachable (Vector v_srv, Vector v_dest, edict_t *pEntity);
int WaypointFindReachable (edict_t *pEntity, float range);
void WaypointPrintInfo (edict_t *pEntity);
void WaypointThink (edict_t *pEntity);
void WaypointFloyds (short *shortest_path, short *from_to);
void WaypointRouteInit (void);
int WaypointRouteFromTo (int src, int dest);
int WaypointDistanceFromTo (int src, int dest);


#endif // WAYPOINT_H
