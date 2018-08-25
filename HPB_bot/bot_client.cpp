//
// HPB bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_client.cpp
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_client.h"
#include "bot_weapons.h"

// types of damage to ignore...
#define IGNORE_DAMAGE (DMG_CRUSH | DMG_FREEZE | DMG_SHOCK | DMG_DROWN | \
                       DMG_NERVEGAS | DMG_RADIATION | DMG_DROWNRECOVER | \
                       DMG_ACID | DMG_SLOWBURN | DMG_SLOWFREEZE)

extern int mod_id;
extern bot_t bots[32];

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions


// This message is sent when the TFC VGUI menu is displayed.
void BotClient_TFC_VGUI(void *p, int bot_index)
{
   if ((*(int *)p) == 2)  // is it a team select menu?

      bots[bot_index].start_action = MSG_TFC_TEAM_SELECT;

   else if ((*(int *)p) == 3)  // is is a class selection menu?

      bots[bot_index].start_action = MSG_TFC_CLASS_SELECT;
}


// This message is sent when the Counter-Strike VGUI menu is displayed.
void BotClient_CS_VGUI(void *p, int bot_index)
{
   if ((*(int *)p) == 2)  // is it a team select menu?

      bots[bot_index].start_action = MSG_CS_TEAM_SELECT;

   else if ((*(int *)p) == 26)  // is is a terrorist model select menu?

      bots[bot_index].start_action = MSG_CS_T_SELECT;

   else if ((*(int *)p) == 27)  // is is a counter-terrorist model select menu?

      bots[bot_index].start_action = MSG_CS_CT_SELECT;
}


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
   {
      bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
   }
   else if (strcmp((char *)p, "#Terrorist_Select") == 0)  // T model select?
   {
      bots[bot_index].start_action = MSG_CS_T_SELECT;
   }
   else if (strcmp((char *)p, "#CT_Select") == 0)  // CT model select menu?
   {
      bots[bot_index].start_action = MSG_CS_CT_SELECT;
   }

   state = 0;  // reset state machine
}


// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
void BotClient_Valve_WeaponList(void *p, int bot_index)
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
      bot_weapon.iFlags = *(int *)p;  // flags for weapon (WTF???)

      // store away this weapon with it's ammo information...
      weapon_defs[bot_weapon.iId] = bot_weapon;

      state = 0;
   }
}

void BotClient_TFC_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_CS_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_Gearbox_WeaponList(void *p, int bot_index)
{
   // this is just like the Valve Weapon List message
   BotClient_Valve_WeaponList(p, bot_index);
}


// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
void BotClient_Valve_CurrentWeapon(void *p, int bot_index)
{
   static int state = 0;   // current state machine state
   static int iState;
   static int iId;
   static int iClip;

   if (state == 0)
   {
      state++;
      iState = *(int *)p;  // state of the current weapon (WTF???)
   }
   else if (state == 1)
   {
      state++;
      iId = *(int *)p;  // weapon ID of current weapon
   }
   else if (state == 2)
   {
      if (iId <= 31)
      {
         iClip = *(int *)p;  // ammo currently in the clip for this weapon

         bots[bot_index].current_weapon.iId = iId;
         bots[bot_index].current_weapon.iClip = iClip;

         // update the ammo counts for this weapon...
         bots[bot_index].current_weapon.iAmmo1 =
            bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo1];
         bots[bot_index].current_weapon.iAmmo2 =
            bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo2];
      }

      state = 0;
   }
}

void BotClient_TFC_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_CS_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_Gearbox_CurrentWeapon(void *p, int bot_index)
{
   // this is just like the Valve Current Weapon message
   BotClient_Valve_CurrentWeapon(p, bot_index);
}


// This message is sent whenever ammo ammounts are adjusted (up or down).
void BotClient_Valve_AmmoX(void *p, int bot_index)
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
      ammount = *(int *)p;  // the ammount of ammo currently available

      bots[bot_index].m_rgAmmo[index] = ammount;  // store it away

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];

      state = 0;
   }
}

void BotClient_TFC_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_CS_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_Gearbox_AmmoX(void *p, int bot_index)
{
   // this is just like the Valve AmmoX message
   BotClient_Valve_AmmoX(p, bot_index);
}


// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_Valve_AmmoPickup(void *p, int bot_index)
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
      ammount = *(int *)p;

      bots[bot_index].m_rgAmmo[index] = ammount;

      ammo_index = bots[bot_index].current_weapon.iId;

      // update the ammo counts for this weapon...
      bots[bot_index].current_weapon.iAmmo1 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
      bots[bot_index].current_weapon.iAmmo2 =
         bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];

      state = 0;
   }
}

void BotClient_TFC_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_CS_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_Gearbox_AmmoPickup(void *p, int bot_index)
{
   // this is just like the Valve Ammo Pickup message
   BotClient_Valve_AmmoPickup(p, bot_index);
}


// This message gets sent when the bot picks up a weapon.
void BotClient_Valve_WeaponPickup(void *p, int bot_index)
{
}

void BotClient_TFC_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_CS_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_Gearbox_WeaponPickup(void *p, int bot_index)
{
   // this is just like the Valve Weapon Pickup message
   BotClient_Valve_WeaponPickup(p, bot_index);
}


// This message gets sent when the bot picks up an item (like a battery
// or a healthkit)
void BotClient_Valve_ItemPickup(void *p, int bot_index)
{
}

void BotClient_TFC_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_CS_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_Gearbox_ItemPickup(void *p, int bot_index)
{
   // this is just like the Valve Item Pickup message
   BotClient_Valve_ItemPickup(p, bot_index);
}


// This message gets sent when the bots health changes.
void BotClient_Valve_Health(void *p, int bot_index)
{
   bots[bot_index].bot_health = *(int *)p;  // health ammount
}

void BotClient_TFC_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}

void BotClient_CS_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}

void BotClient_Gearbox_Health(void *p, int bot_index)
{
   // this is just like the Valve Health message
   BotClient_Valve_Health(p, bot_index);
}


// This message gets sent when the bots armor changes.
void BotClient_Valve_Battery(void *p, int bot_index)
{
   bots[bot_index].bot_armor = *(int *)p;  // armor ammount
}

void BotClient_TFC_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}

void BotClient_CS_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}

void BotClient_Gearbox_Battery(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Battery(p, bot_index);
}


// This message gets sent when the bots are getting damaged.
void BotClient_Valve_Damage(void *p, int bot_index)
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
      damage_origin.z = *(float *)p;

      if ((damage_armor > 0) || (damage_taken > 0))
      {
         // ignore certain types of damage...
         if (damage_bits & IGNORE_DAMAGE)
            return;

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
      }

      state = 0;
   }
}

void BotClient_TFC_Damage(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Damage(p, bot_index);
}

void BotClient_CS_Damage(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Damage(p, bot_index);
}

void BotClient_Gearbox_Damage(void *p, int bot_index)
{
   // this is just like the Valve Battery message
   BotClient_Valve_Damage(p, bot_index);
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

