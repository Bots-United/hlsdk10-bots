/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_econ.cpp,v $
$Revision: 1.6 $
$Date: 2001/05/06 01:48:37 $
$State: Exp $

***********************************

$Log: ai_econ.cpp,v $
Revision 1.6  2001/05/06 01:48:37  eric
Replaced global constants with enums where appropriate

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

#include "ai.h"
#include "ai_behavior.h"
#include "bot.h"

extern bot_t bots[MAX_BOTS];


char *BuyMachinegun (bot_t *pBot) {
	pBot->buy_cat = "5";

	if (pBot->bot_money >= 5750)
		return "1"; // M249
	else
		return NULL;
}

char *BuyAssaultRifle (bot_t *pBot, bool long_distance) {
	pBot->buy_cat = "4";

	bool isTerrorist = (pBot->bot_team == TERRORIST);

	int k = 1;
	if (!long_distance)
		k = 5;

	if ((!isTerrorist) && pBot->bot_money >= 3100 && pBot->movement_style == STEALTH)
		return "3"; // colt
	else if (isTerrorist && pBot->bot_money >= 3500 && !randomize (k))
		return "2";	// SG552
	else if (isTerrorist && pBot->bot_money >= 2500)
		return "1";	// AK-47
	else if ((!isTerrorist) && pBot->bot_money >= 3500 && !randomize (k))
        return "4"; // AUG (NOTE: CS beta 5.2 hasn't this)
	else if ((!isTerrorist) && pBot->bot_money >= 3100)
		return "3"; // colt
	else
		return NULL;
}

char *BuySniperRifle (bot_t *pBot) {
	pBot->buy_cat = "4";

	bool isTerrorist = (pBot->bot_team == TERRORIST);

	if (pBot->bot_money >= 5000 && !randomize (20))
		return "7"; // G3SG1
	else if (pBot->bot_money >= 4750 && randomize (20))
		return "6"; // AWM
	else if (pBot->bot_money >= 2750)
		return "5"; // scout
	else
		return NULL;
}

char *BuyShotgun (bot_t *pBot) {
	pBot->buy_cat = "2";

	if (pBot->bot_money >= 3000 && randomize (20))
		return "2"; // XM1014
	else if (pBot->bot_money >= 1700)
		return "1"; // M3
	else
		return NULL;
}

char *BuySubmachinegun (bot_t *pBot) {
	pBot->buy_cat = "3";

	bool isTerrorist = (pBot->bot_team == TERRORIST);

	if ((!isTerrorist) && pBot->bot_money >= 1250 && (pBot->movement_style == STEALTH || !randomize (99)))
		return "2"; // TMP
	else if (isTerrorist && pBot->bot_money >= 1400 && !randomize (99))
        return "4"; // MAC10 (NOTE: CS beta 5.2 hasn't this weapon)
	else if (pBot->bot_money >= 2350 && randomize (1))
		return "3"; // P90
	else if (pBot->bot_money >= 1500)
		return "1"; // MP5
	else
		return NULL;
}

char *BuySidearm (bot_t *pBot) {
	pBot->buy_cat = "1";

	bool isTerrorist = (pBot->bot_team == TERRORIST);

	if (havePrimaryWeapon (pBot) && randomize (3))
		return NULL;
	else if (isTerrorist && pBot->bot_money >= 500 && pBot->movement_style == STEALTH)
		return "1"; // USP
	else if (pBot->bot_money >= 600 && !randomize (20))
		return "4"; // sig
	else if (pBot->bot_money >= 650)
		return "3"; // deagle
	else if (isTerrorist && pBot->bot_money >= 500)
		return "1"; // USP
	else
		return NULL;
}

char *BuyPrimaryWeapon (bot_t *pBot) {
	char *menu_num = NULL;

	if (!randomize (99))
		menu_num = BuyMachinegun (pBot);

	if (!menu_num) {
		switch (pBot->squad_role) {
		case OVERWATCH:
			if (randomize (2) && pBot->movement_style != RAPID)
				menu_num = BuySniperRifle (pBot);
			if (!randomize (20) && !menu_num)
				menu_num = BuyAssaultRifle (pBot, true);
			break;
				default:
		case MANEUVER:
			if (pBot->movement_style == RAPID && !randomize (10))
				menu_num = BuyShotgun (pBot);
			if (randomize (20) && !menu_num)
				menu_num = BuyAssaultRifle (pBot, false);
			if (!randomize (20) && !menu_num)
				menu_num = BuyShotgun (pBot);
			break;
		}
	}

	if (!menu_num)
		menu_num = BuySubmachinegun (pBot);

	return menu_num;
}

void menuselect (bot_t *pBot, char *num) {
	FakeClientCommand (pBot->pEdict, "menuselect", num, NULL);
}

void BotBuy (bot_t *pBot) {
	bool isTerrorist;

	switch (pBot->start_action) {
	case MSG_CS_BUY_MENU:
		pBot->bot_team = UTIL_GetTeam (pBot->pEdict);

		isTerrorist = (pBot->bot_team == TERRORIST);

		pBot->buy_armor_count++;

		if (pBot->buy_armor_count <= 1) { // only try and buy armor once
			// armor
			if (pBot->pEdict->v.armorvalue < 90 && ((!isTerrorist) || havePrimaryWeapon (pBot) || haveSecondaryWeapon (pBot))) {
				if (pBot->bot_money >= 1000) { // kevlar & helmet
					pBot->start_action = MSG_CS_MENU_WAITING;
					pBot->buy_num = "2";
					menuselect (pBot, "8");
					break;
				} else if (pBot->bot_money >= 650) { // kevlar
					pBot->start_action = MSG_CS_MENU_WAITING;
					pBot->buy_num = "1";
					menuselect (pBot, "8");
					break;
				}
			}
		}

		// primary firearm
		if (! havePrimaryWeapon (pBot)) {
			char *num = BuyPrimaryWeapon (pBot);
			if (num != NULL) {
				pBot->start_action = MSG_CS_MENU_WAITING;
				pBot->buy_num = num;
				menuselect (pBot, pBot->buy_cat);
				pBot->buy_cat = NULL;
				break;
			}
		}

		// sidearm
		if (! haveSecondaryWeapon (pBot)) {
			char *num = BuySidearm (pBot);
			if (num != NULL) {
				pBot->start_action = MSG_CS_MENU_WAITING;
				pBot->buy_num = num;
				menuselect (pBot, pBot->buy_cat);
				pBot->buy_cat = NULL;
				break;
			}
		}

		pBot->buy_ammo_count++;

		if (pBot->buy_ammo_count <= 4) { // attempt to buy exactly four ammo clips
			// ammo
			if (havePrimaryWeapon (pBot)) {
				pBot->start_action = MSG_CS_BUYING;
				menuselect (pBot, "6");
				break;
			} else {
				pBot->start_action = MSG_CS_BUYING;
				menuselect (pBot, "7");
				break;
			}
		}

		pBot->buy_equip_count++;

		// TODO: add grenade purchasing (& NVG)

		if (pBot->buy_equip_count <= 1) { // attempt to buy equip exactly once
			// defuse kit
			// TODO: check to see if bot already has one
			if ((!isTerrorist) && pBot->bot_money >= 200 && randomize (1)) {
				pBot->start_action = MSG_CS_MENU_WAITING;
				pBot->buy_num = "6";
				menuselect (pBot, "8");
				break;
			}
		}

		pBot->start_action = MSG_CS_IDLE;
		menuselect (pBot, "0");
		pBot->buy_num = NULL;
		break;
	case MSG_CS_PISTOL_MENU:
	case MSG_CS_SHOTGUN_MENU:
	case MSG_CS_SMG_MENU:
	case MSG_CS_RIFLE_MENU:
	case MSG_CS_MACHINEGUN_MENU:
	case MSG_CS_EQUIPMENT_MENU:
		pBot->start_action = MSG_CS_BUYING;
		menuselect (pBot, pBot->buy_num);
		pBot->buy_num = NULL;
		break;
	default:
		break;
	}
}

/**
 * Returns true iff the given message
 * is a buying-related message.
 */
bool isBuyMsg (CS_MSG msg) {
	return (msg >= MSG_CS_BUY_MENU);
}
