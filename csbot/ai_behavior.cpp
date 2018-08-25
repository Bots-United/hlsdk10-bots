/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************
Taken from Robert Zubek's paper
entitled "Game Agent Control Using
Parallel Behaviors".
	rob@cs.northwestern.edu
	www.cs.northwestern.edu/~rob/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_behavior.cpp,v $
$Revision: 1.5 $
$Date: 2001/04/30 08:06:10 $
$State: Exp $

***********************************

$Log: ai_behavior.cpp,v $
Revision 1.5  2001/04/30 08:06:10  eric
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
#include "cbase.h"

#include "ai_behavior.h"


/**
 * Returns a tabla rasa behavior.
 */
ai_behavior behavior (float activation, int action) {
	ai_behavior b;
	b.activation = activation;
	b.action = action;
	return b;
}

/**
 * Returns the behavior having the higher
 * activation level.
 */
ai_behavior maximal (ai_behavior b1, ai_behavior b2) {
	if (b1.activation > b2.activation)
		return b1;
	else
		return b2;
}

/**
 * Returns the first behavior if its activation
 * level is anything higher than zero.
 */
ai_behavior subsume (ai_behavior b1, ai_behavior b2) {
	if (b1.activation > 0)
		return b1;
	else
		return b2;
}

/**
 * Returns the input behavior if it's activation
 * level exceeds the threshold, or some default
 * behavior otherwise.
 */
ai_behavior threshold (ai_behavior b, float threshold, ai_behavior d) {
	if (b.activation >= threshold)
		return b;
	else
		return d;
}

/**
 * Produces a smoother version of its input values.
 *
 * @param c  The constant is bound to [0, 1] and
 *     determines how quickly the smoothing responds
 *     to newer values. A constant of zero returns
 *     "x", while a constant of one returns "old".
 */
float mavg (float old, float x, float c) {
	if (c > 1.0)
		c = 1.0;
	else if (c < 0.0)
		c = 0.0;

	return ((c * old) + ((1 - c) * x));
}

/**
 * Produces a smoother version of its input values.
 *
 * @param all  Array values are expected to be in
 *     chronological order (i.e. the newest item
 *     is at the end of the array).
 */
float mavg (float all[], int size, float c) {
	if (size < 1)
		return 0.0; // TODO: throw exception
	if (size < 2)
		return all[0];

	float temp = all[0];
	for (int i = 1; i < size; i++)
		temp = mavg (temp, all[i], c);
	return temp;
}

/**
 * Given some values min and max, clamp the inputs
 * to be inside the [min, max] range.
 */
float clamp (float x, float min, float max) {
	if (x <= min)
		return min;
	else if (x >= max)
		return max;
	else
		return x;
}

/**
 * Given some values min and max, clamp the inputs to
 * be the last known value outside the [min, max] range.
 */
float hysteresis (float old, float x, float min, float max) {
	if (x <= min || x >= max)
		return x;
	else
		return old;
}

/**
 * Given some values min and max, return the boolean
 * equivalent of the last known value outside the
 * [min, max] range.
 */
bool hysteresis (bool old, float x, float min, float max) {
	if (x <= min)
		return false;
	else if (x >= max)
		return true;
	else
		return old;
}

/**
 * Given a series of boolean inputs, return true only
 * the first time the input switches from false to true.
 */
bool up_edge (bool old, bool x) {
	return (old == false && x == true);
}

/**
 * Given a series of boolean inputs, return true only
 * the first time the input switches from true to false.
 */
bool down_edge (bool old, bool x) {
	return (old == true && x == false);
}

/** Returns false (1 / X+1) percent of the time. */
bool randomize (int x) {
	int random = RANDOM_LONG (0, x);
	if (random == 0)
		return false;
	else
		return true;
}
