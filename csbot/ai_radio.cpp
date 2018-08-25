/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_radio.cpp,v $
$Revision: 1.6 $
$Date: 2001/05/02 11:51:03 $
$State: Exp $

***********************************

$Log: ai_radio.cpp,v $
Revision 1.6  2001/05/02 11:51:03  eric
Improved cover seeking

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

#include "bot.h"


/**
 * Returns true iff the given radio message
 * is the current one waiting to be broadcast.
 */
bool isCurrentRadio (bot_t *pBot, char *radio_type, char *radio_num) {
	if (pBot->radio_type == NULL || pBot->radio_num == NULL)
		return false;

	return (strcmp (pBot->radio_type, radio_type) == 0 && strcmp (pBot->radio_num, radio_num) == 0);
}

/**
 * Instructs the bot to use the radio soon.
 * When multiple radio commands are given,
 * the first requested has priority unless
 * the override parameter is true.
 */
void radio (bot_t *pBot, float delay, char *radio_type, char *radio_num, bool override) {
	if (pBot->radio_time < gpGlobals->time || override) {
		pBot->radio_time = gpGlobals->time + delay;
		pBot->radio_type = radio_type;
		pBot->radio_num = radio_num;
	}
}

/**
 * Queues the given radio command,
 * regardless of if one already exists.
 */
void radioOverride (bot_t *pBot, float delay, char *radio_type, char *radio_num) {
	radio (pBot, delay, radio_type, radio_num, true);
}

/**
 * Queues the given radio command so
 * long as no other message is already
 * on the queue.
 */
void radio (bot_t *pBot, float delay, char *radio_type, char *radio_num) {
	radio (pBot, delay, radio_type, radio_num, false);
}

/** Actually issues the radio command already specified. */
bool BotRadio (bot_t *pBot) {
	if (pBot->radio_time < gpGlobals->time) {
		edict_t *pEdict = pBot->pEdict;

		if (pBot->radio_type != NULL && pBot->radio_num != NULL) {

			char *radio_type = "radio3"; // default to three
			switch (atoi (pBot->radio_type)) {
				case 1:
					radio_type = "radio1";
					break;
				case 2:
					radio_type = "radio2";
					break;
				case 3:
					radio_type = "radio3";
					break;
			}

			FakeClientCommand (pEdict, radio_type, NULL, NULL);
			FakeClientCommand (pEdict, "menuselect", pBot->radio_num, NULL);

			pBot->radio_type = NULL;
			pBot->radio_num = NULL;		

			return true;
		}
	}

	return false;
}

/** Given bot says message aloud. */
bool speak (bot_t *pBot, char *msg) {
	if (pBot->speak_time < gpGlobals->time) {
		UTIL_HostSay (pBot->pEdict, 0, msg);
		pBot->speak_time = gpGlobals->time + 1.0;
		return true;
	}

	return false;
}
