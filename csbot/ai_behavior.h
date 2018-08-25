/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_behavior.h,v $
$Revision: 1.4 $
$Date: 2001/04/30 08:06:10 $
$State: Exp $

***********************************

$Log: ai_behavior.h,v $
Revision 1.4  2001/04/30 08:06:10  eric
willFire() deals with crouching versus standing

Revision 1.3  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/
//
// ai_behavior.h
//

#ifndef AI_BEHAVIOR_H
#define AI_BEHAVIOR_H


typedef struct
{
   float activation;
   int action;
} ai_behavior;


ai_behavior behavior (float activation, int action);
ai_behavior maximal (ai_behavior b1, ai_behavior b2);
ai_behavior subsume (ai_behavior b1, ai_behavior b2);
ai_behavior threshold (ai_behavior b, float threshold, ai_behavior d);
float mavg (float old, float x, float c);
float mavg (float all[], int size, float c);
float clamp (float x, float min, float max);
float hysteresis (float old, float x, float min, float max);
bool hysteresis (bool old, float x, float min, float max);
bool up_edge (bool old, bool x);
bool down_edge (bool old, bool x);
bool randomize (int x);


#endif // AI_BEHAVIOR_H
