/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************
Taken from William van der Sterren's
paper entitled "CGF Terrain Specific
AI - Terrain analysis".
	william@botepidemic.com
	www.botepidemic.com/aid/cgf/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_terrain.h,v $
$Revision: 1.9 $
$Date: 2001/05/04 08:18:17 $
$State: Exp $

***********************************

$Log: ai_terrain.h,v $
Revision 1.9  2001/05/04 08:18:17  eric
CGF-style vision stubs

Revision 1.8  2001/05/02 13:09:31  eric
Housekeeping

Revision 1.7  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.6  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

Revision 1.5  2001/04/30 10:44:20  eric
Tactical terrain evaluation fundamentals

Revision 1.4  2001/04/30 08:06:10  eric
willFire() deals with crouching versus standing

Revision 1.3  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef AI_TERRAIN_H
#define AI_TERRAIN_H

#include "waypoint.h"

/**
 * Represents an influence mapping
 * between two waypoints.
 *<p>
 * Value is bound over [0,1]; where
 * zero represents maximal disadvantage
 * and one represents maximal advantage.
 */
#define INFLUENCE float

/**
 * The intrinsic and influencial tactical
 * value of a waypoint.
 *<p>
 * Intrinsic tactical value:
 * (1) light level
 * (2) movement speed (crouch, jump, ladder, lift, door)
 *<p>
 * Influence value:
 * (1) access advantage (hard to reach from locations it overlooks)
 * (2) cover advantage (nearby cover from potential threats in overlooked area)
 * (3) visibility advantage (less lighting than overlooked lighting)
 * (4) 'no cover' advantage (overlooked do not have cover)
 * (5) speed advantage (overlooked have impeded movement)
 * (6) surprise advantage (number of access routes that bring threats nearby w/o being observed)
 * (7) height advantage (above potential threats is better)
 */
typedef struct {
	WP_INDEX index;
	float avg_v_distance;
	float light_level;		// 0 = no light whatsoever, 1 = fully lit
	float movement_speed;	// 0 = impassable, 1 = no impedence
	INFLUENCE access;		// 0 = fully accessible, 1 = in-accessible
	INFLUENCE cover;		// 0 = no nearby cover, 1 = full cover available
	INFLUENCE visibility;
	INFLUENCE no_cover;
	INFLUENCE speed;
	INFLUENCE surprise;
	INFLUENCE elevation;	// 0 = far below overlooked, 1 = far above overlooked
	float ambush;
	float sniping;
	float stealth;
	float travel;
} TERRAIN;


float terrainDistance (WP_INDEX src, WP_INDEX dest);
void terrainInit (TERRAIN *t, WP_INDEX wp);
void terrainReset (TERRAIN *t);
void TerrainInitAll (void);
void WaypointPrintTerrainInfo (edict_t *pEntity, int index);
int coverCount (int wp_me, int wp_threat);
int WaypointCover (bot_t *pBot, int wp_threat, float range, int result[], unsigned int result_size);
int WaypointChoke (int wp_src, int result[], unsigned int result_size);
int WaypointAmbush (bot_t *pBot, int wp_target, float range, int result[], unsigned int result_size);


#endif // AI_TERRAIN_H
