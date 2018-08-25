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

$Source: /usr/local/cvsroot/csbot/src/dlls/bot_client.cpp,v $
$Revision: 1.6 $
$Date: 2001/04/30 13:39:21 $
$State: Exp $

***********************************

$Log: bot_client.cpp,v $
Revision 1.6  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

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
#include "cbase.h"

#include "ai.h"
#include "ai_squad_func.h"
#include "bot.h"
#include "bot_func.h"
#include "bot_client.h"
#include "bot_weapons.h"

// types of damage to ignore...
#define IGNORE_DAMAGE (DMG_CRUSH | DMG_FREEZE | DMG_FALL | DMG_SHOCK | \
                       DMG_DROWN | DMG_NERVEGAS | DMG_RADIATION | \
                       DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN | \
                       DMG_SLOWFREEZE | 0xFF000000)

extern bot_t bots[32];

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions


// This message is sent when a menu is being displayed in Counter-Strike.
void BotClient_CS_ShowMenu(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (state < 3)
   {
      state++;  // ignore first 3 fields of message
      return;
   }

   if (strcmp((char *)p, "#Team_Select") == 0)  // team select menu?
      bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
   else if (strcmp((char *)p, "#Terrorist_Select") == 0)  // T model select?
      bots[bot_index].start_action = MSG_CS_T_SELECT;
   else if (strcmp((char *)p, "#CT_Select") == 0)  // CT model select menu?
      bots[bot_index].start_action = MSG_CS_CT_SELECT;
   else if (strcmp((char *)p, "#Buy") == 0)
      bots[bot_index].start_action = MSG_CS_BUY_MENU;
   else if (strcmp((char *)p, "#BuyItem") == 0)
      bots[bot_index].start_action = MSG_CS_EQUIPMENT_MENU;
   else if (strcmp((char *)p, "#DCT_BuyItem") == 0)
      bots[bot_index].start_action = MSG_CS_EQUIPMENT_MENU;
   else if (strcmp((char *)p, "#DT_BuyItem") == 0)
      bots[bot_index].start_action = MSG_CS_EQUIPMENT_MENU;
   else if (strcmp((char *)p, "#BuyPistol") == 0)
      bots[bot_index].start_action = MSG_CS_PISTOL_MENU;
   else if (strcmp((char *)p, "#AS_BuyShotgun") == 0)
      bots[bot_index].start_action = MSG_CS_SHOTGUN_MENU;
   else if (strcmp((char *)p, "#BuyShotgun") == 0)
      bots[bot_index].start_action = MSG_CS_SHOTGUN_MENU;
   else if (strcmp((char *)p, "#T_BuyRifle") == 0)
      bots[bot_index].start_action = MSG_CS_RIFLE_MENU;
   else if (strcmp((char *)p, "#CT_BuyRifle") == 0)
      bots[bot_index].start_action = MSG_CS_RIFLE_MENU;
   else if (strcmp((char *)p, "#AS_T_BuyRifle") == 0)
      bots[bot_index].start_action = MSG_CS_RIFLE_MENU;
   else if (strcmp((char *)p, "#AS_CT_BuyRifle") == 0)
      bots[bot_index].start_action = MSG_CS_RIFLE_MENU;
   else if (strcmp((char *)p, "#CT_BuySubMachineGun") == 0)
      bots[bot_index].start_action = MSG_CS_SMG_MENU;
   else if (strcmp((char *)p, "#T_BuySubMachineGun") == 0)
      bots[bot_index].start_action = MSG_CS_SMG_MENU;
   else if (strcmp((char *)p, "#AS_CT_BuySubMachineGun") == 0)
      bots[bot_index].start_action = MSG_CS_SMG_MENU;
   else if (strcmp((char *)p, "#AS_T_BuySubMachineGun") == 0)
      bots[bot_index].start_action = MSG_CS_SMG_MENU;
   else if (strcmp((char *)p, "#AS_T_BuyMachineGun") == 0)
      bots[bot_index].start_action = MSG_CS_MACHINEGUN_MENU;
   else if (strcmp((char *)p, "#BuyMachineGun") == 0)
      bots[bot_index].start_action = MSG_CS_MACHINEGUN_MENU;

   state = 0;  // reset state machine
}


// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
void BotClient_CS_WeaponList(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static bot_weapon_t bot_weapon;

   if (state == 0)
   {
      state++;
      strcpy(bot_weapon.szClassname, (char *)p);
   }
   else if (state == 1)
   {
      state++;
      bot_weapon.iAmmo1 = *(int *)p;  // ammo index 1
   }
   else if (state == 2)
   {
      state++;
      bot_weapon.iAmmo1Max = *(int *)p;  // max ammo1
   }
   else if (state == 3)
   {
      state++;
      bot_weapon.iAmmo2 = *(int *)p;  // ammo index 2
   }
   else if (state == 4)
   {
      state++;
      bot_weapon.iAmmo2Max = *(int *)p;  // max ammo2
   }
   else if (state == 5)
   {
      state++;
      bot_weapon.iSlot = *(int *)p;  // slot for this weapon
   }
   else if (state == 6)
   {
      state++;
      bot_weapon.iPosition = *(int *)p;  // position in slot
   }
   else if (state == 7)
   {
      state++;
      bot_weapon.iId = *(int *)p;  // weapon ID
   }
   else if (state == 8)
   {
      state = 0;

      bot_weapon.iFlags = *(int *)p;  // flags for weapon (WTF???)

      // store away this weapon with it's ammo information...
      weapon_defs[bot_weapon.iId] = bot_weapon;
   }
}

// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
void BotClient_CS_CurrentWeapon(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int iState;
   static int iId;
   static int iClip;

   if (state == 0)
   {
      state++;
      iState = *(int *)p;  // state of the current weapon
   }
   else if (state == 1)
   {
      state++;
      iId = *(int *)p;  // weapon ID of current weapon
   }
   else if (state == 2)
   {
      state = 0;

      iClip = *(int *)p;  // ammo currently in the clip for this weapon

      if (iId <= 31)
      {
         if (iState == 1)
         {
            bots[bot_index].current_weapon.iId = iId;
            bots[bot_index].current_weapon.iClip = iClip;

            // update the ammo counts for this weapon...
            bots[bot_index].current_weapon.iAmmo1 = bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo1];
            bots[bot_index].current_weapon.iAmmo2 = bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo2];
         }
      }
   }
}


// This message is sent whenever ammo ammounts are adjusted (up or down).
void BotClient_CS_AmmoX(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int index;
   static int ammount;
   int ammo_index;

   if (state == 0)
   {
      state++;
      index = *(int *)p;  // ammo index (for type of ammo)
   }
   else if (state == 1)
   {
      state = 0;

      ammount = *(int *)p;  // the ammount of ammo currently available

      bots[bot_index].m_rgAmmo[index] = ammount;  // store it away

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
   }
}


// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_CS_AmmoPickup(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int index;
   static int ammount;
   int ammo_index;

   if (state == 0)
   {
      state++;
      index = *(int *)p;
   }
   else if (state == 1)
   {
      state = 0;

      ammount = *(int *)p;

      bots[bot_index].m_rgAmmo[index] = ammount;

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 = bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
   }
}


// This message gets sent when the bots are getting damaged.
void BotClient_CS_Damage(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int damage_armor;
   static int damage_taken;
   static int damage_bits;  // type of damage being done
   static Vector damage_origin;

   if (state == 0)
   {
      state++;
      damage_armor = *(int *)p;
   }
   else if (state == 1)
   {
      state++;
      damage_taken = *(int *)p;
   }
   else if (state == 2)
   {
      state++;
      damage_bits = *(int *)p;
   }
   else if (state == 3)
   {
      state++;
      damage_origin.x = *(float *)p;
   }
   else if (state == 4)
   {
      state++;
      damage_origin.y = *(float *)p;
   }
   else if (state == 5)
   {
      state = 0;

      damage_origin.z = *(float *)p;

      if ((damage_armor > 0) || (damage_taken > 0))
      {
         // ignore certain types of damage...
         if (damage_bits & IGNORE_DAMAGE)
            return;
/*
         // if the bot doesn't have an enemy and someone is shooting at it then
         // turn in the attacker's direction...
         if (bots[bot_index].pBotEnemy == NULL)
         {
            // face the attacker...
            Vector v_enemy = damage_origin - bots[bot_index].pEdict->v.origin;
            Vector bot_angles = UTIL_VecToAngles( v_enemy );

            bots[bot_index].pEdict->v.ideal_yaw = bot_angles.y;

            BotFixIdealYaw(bots[bot_index].pEdict);
         }
*/
      }
   }
}

// This message gets sent when the bots money ammount changes (for CS)
void BotClient_CS_Money(void *p, int bot_index)
{
   static int state = 0;   // current state machine state

   if (state == 0)
   {
      state++;

      bots[bot_index].bot_money = *(int *)p;  // amount of money
   }
   else
   {
      state = 0;  // ingore this field
   }
}


// This message gets sent when the bots get killed
void BotClient_CS_DeathMsg(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int killer_index;
   static int victim_index;
   static edict_t *victim_edict;
   static int index;

   if (state == 0)
   {
      state++;
      killer_index = *(int *)p;  // ENTINDEX() of killer
   }
   else if (state == 1)
   {
      state++;
      victim_index = *(int *)p;  // ENTINDEX() of victim
   }
   else if (state == 2)
   {
      state = 0;

      victim_edict = INDEXENT(victim_index);

      index = UTIL_GetBotIndex(victim_edict);

      // is this message about a bot being killed?
      if (index != -1)
      {
         if ((killer_index == 0) || (killer_index == victim_index))
         {
            // bot killed by world (worldspawn) or bot killed self...
            bots[index].killer_edict = NULL;
         }
         else
         {
            // store edict of player that killed this bot...
            bots[index].killer_edict = INDEXENT(killer_index);
         }
      }
   }
}

void BotClient_CS_ScreenFade(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int duration;
   static int hold_time;
   static int fade_flags;
   int length;

   if (state == 0)
   {
      state++;
      duration = *(int *)p;
   }
   else if (state == 1)
   {
      state++;
      hold_time = *(int *)p;
   }
   else if (state == 2)
   {
      state++;
      fade_flags = *(int *)p;
   }
   else if (state == 6)
   {
      state = 0;

      length = (duration + hold_time) / 4096;
      bots[bot_index].blinded_time = gpGlobals->time + length - 2.0;
   }
   else
   {
      state++;
   }
}

// this message is sent when receiving a radio message
// TODO: when else?
void BotClient_CS_TextMsg(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static char *sender_name = "";
	static char *msg = "";

	if (!strncmp((char *)p, "%s (RADIO): %s", 14)) { // is radio message
		state = 1;
	} else if (state == 1) {
		state = 2;
		sender_name = (char *)p; // example: everyman
	} else if (state == 2) {
		state = 0;

		msg = (char *)p; // example: #Stick_together_team

		if (msg == NULL || sender_name == NULL || bot_index < 0)
			return;

		/*
		fp=fopen ("bot.txt","a");
		fprintf (fp,"MSG_FROM (bot_index = %i): %s\n", bot_index, sender_name);
		fprintf (fp,"MSG (bot_index = %i): %s\n", bot_index, msg);
		fclose (fp);
		*/

		// receiver
		bot_t *pBot = &bots[bot_index];
		edict_t *pEdict = pBot->pEdict;

		edict_t *sender = NULL;
		for (int i = 1; i <= gpGlobals->maxClients; i++) {
			edict_t *client = INDEXENT (i);

			if (client == NULL || client->free)
				continue;

			if (! strcmp (sender_name, STRING (client->v.netname))) {
				sender = client;
				break;
			}
		}


		bool senderIsBot = (UTIL_GetBotPointer (sender) != NULL);

		if (senderIsBot && !strncmp (msg, "Enemy down.", 11))
			pBot->radio_time_down = gpGlobals->time;

		if (sender == NULL
			|| sender == pEdict
			|| !IsAlive (sender))
		{
			// Ignore the sender.
			return;
		}
		else if (((! senderIsBot)
				&& FVisible (sender->v.origin, pEdict)
				&& distance (sender->v.origin, pEdict->v.origin) < RADIO_RANGE)
			|| (senderIsBot
				&& sameSquad (pBot, sender)))
		{
			if (!strncmp(msg, "Stick together, team.", 21) || !strncmp(msg, "Regroup Team.", 13)) {
			} else if (!strncmp(msg, "You Take the Point.", 19)) {
			} else if (!strncmp(msg, "Cover me!", 9) || !strncmp (msg, "Follow Me.", 9)) {
				// TODO: prez should just simply radio this at start (human?)
			} else if (!strncmp(msg, "Get in position and wait for my go.", 35) || !strncmp (msg, "Hold This Position.", 19)) {
				ambush (pBot);
			} else if (!strncmp(msg, "Report in, team.", 16)) {
				radio (pBot, RANDOM_FLOAT (1.5, 3.5), "3", "6"); // reporting in
			} else if (!strncmp(msg, "Need backup.", 12) || !strncmp (msg, "Taking Fire.. Need Assistance!", 30)) {
			} else if (!strncmp(msg, "Team, fall back!", 16)) {
				radio (pBot, RANDOM_FLOAT (1.0, 4.0), "3", "1"); // affirmitive
				setMovementStyle (pBot, RETREAT, RANDOM_FLOAT (10.0, 40.0));
			} else if (!strncmp(msg, "Go go go!", 9) || !strncmp(msg, "Storm the Front!", 16)) {
				setMovementStyle (pBot, RAPID, RANDOM_FLOAT (10.0, 40.0));
			}
		}
	}
}
