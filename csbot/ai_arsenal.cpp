/**********************************
TheBot
   Eric Bieschke's CS Bot
   http://gamershomepage.com/thebot/
**********************************
Adapted from:
   HPB_bot
   botman's High Ping Bastard bot
   http://planethalflife.com/botman/
***********************************

$Source: /usr/local/cvsroot/csbot/src/dlls/ai_arsenal.cpp,v $
$Revision: 1.8 $
$Date: 2001/05/03 19:35:16 $
$State: Exp $

***********************************

$Log: ai_arsenal.cpp,v $
Revision 1.8  2001/05/03 19:35:16  eric
CGF-style aiming

Revision 1.7  2001/05/02 13:09:31  eric
Housekeeping

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

#include "ai.h"
#include "ai_behavior.h"
#include "bot_weapons.h"

extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern bool b_botdontshoot;


typedef struct
{
	int iId;
	char weapon_name[64];
	float min_distance;
	float max_distance;
	float aim_head; // [0,1]: how often to aim at head
	float min_delay;
	float max_delay;
	float min_hold;
	float max_hold;
} bot_weapon_select_t;

bot_weapon_select_t weapon_select[] = {
   {CS_WEAPON_AK47, "weapon_ak47", 0.0, 1200.0, 0.1, 0.6, 0.8, 0.0, 3.0},
   {CS_WEAPON_AUG, "weapon_aug", 0.0, 2000.0, 0.2, 0.3, 0.6, 0.1, 2.0},
   {CS_WEAPON_AWP, "weapon_awp", 0.0, 3200.0, 0.0, 0.5, 2.0, 0.0, 0.0},
   {CS_WEAPON_DEAGLE, "weapon_deagle", 0.0, 800.0, 0.3, 0.1, 0.2, 0.0, 0.0},
   {CS_WEAPON_G3SG1, "weapon_g3sg1", 0.0, 3200.0, 0.1, 0.6, 0.8, 0.0, 0.5},
   {CS_WEAPON_GLOCK18, "weapon_glock18", 0.0, 800.0, 0.4, 0.1, 0.2, 0.0, 0.0},
   {CS_WEAPON_KNIFE, "weapon_knife", 0.0, 50.0, 0.8, 0.1, 0.4, 0.0, 0.0},
   {CS_WEAPON_M249, "weapon_m249", 0.0, 1200.0, 0.2, 0.2, 0.8, 0.0, 0.0},
   {CS_WEAPON_M3, "weapon_m3", 0.0, 400.0, 0.0, 0.2, 0.7, 0.0, 0.0},
   {CS_WEAPON_M4A1, "weapon_m4a1", 0.0, 1200.0, 0.3, 0.2, 0.5, 0.0, 0.5},
   {CS_WEAPON_MAC10, "weapon_mac10", 0.0, 1200.0, 0.2, 0.2, 0.8, 0.2, 1.0},
   {CS_WEAPON_MP5NAVY, "weapon_mp5navy", 0.0, 1200.0, 0.3, 0.2, 0.6, 0.1, 1.0},
   {CS_WEAPON_P228, "weapon_p228", 0.0, 800.0, 0.4, 0.1, 0.2, 0.0, 0.0},
   {CS_WEAPON_P90, "weapon_p90", 0.0, 1200.0, 0.3, 0.2, 0.8, 0.5, 4.0},
   {CS_WEAPON_SCOUT, "weapon_scout", 0.0, 3200.0, 0.2, 0.5, 1.5, 0.0, 0.0},
   {CS_WEAPON_SG552, "weapon_sg552", 0.0, 2000.0, 0.2, 0.6, 1.0, 0.0, 0.5},
   {CS_WEAPON_TMP, "weapon_tmp", 0.0, 1200.0, 0.3, 0.2, 0.6, 0.4, 3.0},
   {CS_WEAPON_USP, "weapon_usp", 0.0, 800.0, 0.4, 0.1, 0.2, 0.0, 0.0},
   {CS_WEAPON_XM1014, "weapon_xm1014", 0.0, 400.0, 0.0, 0.3, 0.8, 0.0, 0.0},
   /* terminator */
   {0, "", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
};

/** Returns a handle on the current weapon. */
bot_weapon_select_t *returnCurrent (bot_t *pBot) {
	int i = 0;
	bot_weapon_select_t *pSelect = &weapon_select[i];
	while (pSelect->iId) {
		if (pBot->current_weapon.iId == pSelect->iId) {
			break;
		} else {
			i++;
			pSelect = &weapon_select[i];
		}
	}

	return pSelect;
}

/** Returns true iff current weapon is the knife. */
bool usingKnife (bot_t *pBot) {
	return (pBot->current_weapon.iId == CS_WEAPON_KNIFE);
}

/**
 * Returns true iff the current weapon
 * is capable of suppressionary fire.
 */
bool suppressionFire (bot_t *pBot) {
	bot_weapon_select_t *pCurrent = returnCurrent (pBot);
	return (pCurrent->max_hold > 0.0 && pBot->squad_role == OVERWATCH);
}

/**
 * Returns true if the bot should
 * aim for the threat's head.
 */
bool aimHead (bot_t *pBot) {
	bot_weapon_select_t *pCurrent = returnCurrent (pBot);
	return (RANDOM_FLOAT (0, 1) <= pCurrent->aim_head);
}

/**
 * Returns true iff the bot is within firing
 * range with the current weapon.
 */
bool inRange (bot_t *pBot) {
	if (pBot->threat == NULL)
		return false;

	float d = distance (pBot->threat->v.origin, pBot->pEdict->v.origin);
	bot_weapon_select_t *pCurrent = returnCurrent (pBot);
	return (d >= pCurrent->min_distance && d <= pCurrent->max_distance);
}

/**
 * Returns true iff the bot has ammo
 * for his current gun.
 */
bool haveAmmo (bot_t *pBot) {
	return (pBot->current_weapon.iAmmo1 >= 1);
}

/** 
 * Returns true iff there is ammo 
 * for the current gun in the clip.
 */
bool ammoInClip (bot_t *pBot) {
	return (pBot->current_weapon.iClip >= 1);
}

/**
 * Returns TRUE iff the fire trajectory
 * hits something other than it's target.
 *<p>
 * TODO: only own team should impede shooting
 */
bool traceFire (bot_t *pBot, Vector v_src) {
	TraceResult tr;

	Vector v_dest = pBot->threat->v.origin;
	Vector lead = ((v_dest - v_src).Normalize ()) * 23.0; // ~23 is the diagonal radius of a square character
	v_src = v_src + lead;
	v_dest = v_dest - lead;

	UTIL_TraceLine (v_src, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);
	if (tr.flFraction < 1.0) // trace hit
		return true;
	else
		return false;
}

/**
 * Returns a bitmask indicates which
 * states the bot is willing to open fire in. 
 */
int willFire (bot_t *pBot) {
	int fire_bitmask = 0;

	if (pBot->threat == NULL || (!ammoInClip (pBot) && !usingKnife (pBot)))
		return fire_bitmask;

	if (suppressionFire (pBot))
		fire_bitmask |= SUPPRESSIVE;

	if (!inRange (pBot) || !inSight (pBot, pBot->threat))
		return fire_bitmask;

	if (! traceFire (pBot, eyePosition (pBot->pEdict)))
		fire_bitmask |= STANDING_CLEAR;

	if (! traceFire (pBot, pBot->pEdict->v.origin))
		fire_bitmask |= CROUCH_CLEAR;

	// TODO: add JUMPING_CLEAR

	return fire_bitmask;
}

/** Returns true iff the current weapon was fired. */
bool openFire (bot_t *pBot) {
	if (pBot->threat == NULL)
		return false;

	bot_weapon_select_t *pCurrent = returnCurrent (pBot);

	if (pBot->shoot_time <= gpGlobals->time) {// shoot_time has passed
		if (pBot->shoot_stop_time <= gpGlobals->time) {// shoot_stop_time has passed
			if (pBot->shoot_time > pBot->shoot_stop_time) { // shoot_time is after shoot_stop_time
				pBot->shoot_stop_time = gpGlobals->time + RANDOM_FLOAT (pCurrent->min_hold, pCurrent->max_hold);
				if (! b_botdontshoot)
					pBot->pEdict->v.button |= IN_ATTACK;
				return true;
			} else {
				pBot->shoot_time = gpGlobals->time + RANDOM_FLOAT (pCurrent->min_delay, pCurrent->max_delay);
				return false;
			}
		} else {
			if (! b_botdontshoot)
				pBot->pEdict->v.button |= IN_ATTACK;
			return true;
		}
	}

	return false;
}

/** 
 * Returns true iff the given weapon
 * is in the bot's possession.
 */
bool haveWeapon (bot_t *pBot, int weapon) {
	return (pBot->pEdict->v.weapons & (1 << weapon)) != 0;
}

/** Returns true iff the bot is carrying a primary weapon. */
bool havePrimaryWeapon (bot_t *pBot) {
	if (haveWeapon (pBot, CS_WEAPON_AK47)
		|| haveWeapon (pBot, CS_WEAPON_AUG)
		|| haveWeapon (pBot, CS_WEAPON_AWP)
		|| haveWeapon (pBot, CS_WEAPON_G3SG1)
		|| haveWeapon (pBot, CS_WEAPON_M249)
		|| haveWeapon (pBot, CS_WEAPON_M3)
		|| haveWeapon (pBot, CS_WEAPON_M4A1)
		|| haveWeapon (pBot, CS_WEAPON_MAC10)
		|| haveWeapon (pBot, CS_WEAPON_MP5NAVY)
		|| haveWeapon (pBot, CS_WEAPON_P90)
		|| haveWeapon (pBot, CS_WEAPON_SCOUT)
		|| haveWeapon (pBot, CS_WEAPON_SG552)
		|| haveWeapon (pBot, CS_WEAPON_TMP)
		|| haveWeapon (pBot, CS_WEAPON_XM1014))
	{
		return true;
	}

	return false;
}

/**
 * Returns true iff the bot is carrying a sidearm.
 * Default sidearms return false.
 */
bool haveSecondaryWeapon (bot_t *pBot) {
	bool isTerrorist = (pBot->bot_team == TERRORIST);

	if (haveWeapon (pBot, CS_WEAPON_P228)
		|| (haveWeapon (pBot, CS_WEAPON_USP) && isTerrorist)
		|| (haveWeapon (pBot, CS_WEAPON_GLOCK18) && !isTerrorist)
		|| haveWeapon (pBot, CS_WEAPON_DEAGLE))
	{
		return true;
	}
	
	return false;
}

/** Returns true iff the given entity is a primary weapon. */
bool isPrimaryWeapon (edict_t *pent) {
	char item_name[40];
	strcpy (item_name, STRING(pent->v.classname));

	if (! strcmp ("weapon_ak47", item_name))
		return true;
	else if (! strcmp ("weapon_aug", item_name))
		return true;
	else if (! strcmp ("weapon_awp", item_name))
		return true;
	else if (! strcmp ("weapon_g3sg1", item_name))
		return true;
	else if (! strcmp ("weapon_m249", item_name))
		return true;
	else if (! strcmp ("weapon_m3", item_name))
		return true;
	else if (! strcmp ("weapon_m4a1", item_name))
		return true;
	else if (! strcmp ("weapon_mac10", item_name))
		return true;
	else if (! strcmp ("weapon_mp5navy", item_name))
		return true;
	else if (! strcmp ("weapon_p90", item_name))
		return true;
	else if (! strcmp ("weapon_scout", item_name))
		return true;
	else if (! strcmp ("weapon_sg550", item_name))
		return true;
	else if (! strcmp ("weapon_sg552", item_name))
		return true;
	else if (! strcmp ("weapon_tmp", item_name))
		return true;
	else if (! strcmp ("weapon_ump45", item_name))
		return true;
	else if (! strcmp ("weapon_xm1014", item_name))
		return true;

	return false;
}

/** Returns true iff the given entity is a weapon lying on the ground. */
bool isWeapon (edict_t *pent) {
	if (pent->v.owner != NULL
		&& (! strncmp (STRING (pent->v.classname), "weapon_", strlen ("weapon_")))
		&& (! strcmp (STRING (pent->v.owner->v.classname), "weaponbox"))
		)
		return true;
	else
		return false;
}

/**
 * Changes to the next weapon.
 *<p>
 * Taken from TEAMbot 4.5 by
 * Alistair Stewart.
 */
void changeWeapon (bot_t *pBot) {
	edict_t *pEdict = pBot->pEdict;

	switch (pBot->current_weapon.iId) {
		case CS_WEAPON_AK47:
		case CS_WEAPON_AUG:
		case CS_WEAPON_AWP:
		case CS_WEAPON_G3SG1:
		case CS_WEAPON_M249:
		case CS_WEAPON_M3:
		case CS_WEAPON_M4A1:
		case CS_WEAPON_MAC10:
		case CS_WEAPON_MP5NAVY:
		case CS_WEAPON_P90:
		case CS_WEAPON_SCOUT:
		case CS_WEAPON_SG552:
		case CS_WEAPON_TMP:
		case CS_WEAPON_XM1014:
			if (haveWeapon (pBot, CS_WEAPON_P228))
				UTIL_SelectItem (pEdict, "weapon_p228");
			else if (haveWeapon (pBot, CS_WEAPON_USP))
				UTIL_SelectItem (pEdict, "weapon_usp");
			else if (haveWeapon (pBot, CS_WEAPON_GLOCK18))
				UTIL_SelectItem (pEdict, "weapon_glock18");
			else if (haveWeapon (pBot, CS_WEAPON_DEAGLE))
				UTIL_SelectItem (pEdict, "weapon_deagle");
			else
				UTIL_SelectItem (pEdict, "weapon_knife");
			break;
		default:
		case CS_WEAPON_DEAGLE:
		case CS_WEAPON_GLOCK18:
		case CS_WEAPON_P228:
		case CS_WEAPON_USP:
			UTIL_SelectItem (pEdict, "weapon_knife");
			break;
	}
}
