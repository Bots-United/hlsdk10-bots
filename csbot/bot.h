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

$Source: /usr/local/cvsroot/csbot/src/dlls/bot.h,v $
$Revision: 1.11 $
$Date: 2001/05/11 07:06:19 $
$State: Exp $

***********************************

$Log: bot.h,v $
Revision 1.11  2001/05/11 07:06:19  eric
bots respond to being stuck

Revision 1.10  2001/05/10 10:00:25  eric
WP_INVALID usage

Revision 1.9  2001/05/06 01:48:37  eric
Replaced global constants with enums where appropriate

Revision 1.8  2001/05/03 19:35:16  eric
CGF-style aiming

Revision 1.7  2001/05/02 11:51:03  eric
Improved cover seeking

Revision 1.6  2001/04/30 13:39:21  eric
Primitive cover seeking behavior

Revision 1.5  2001/04/30 08:06:11  eric
willFire() deals with crouching versus standing

Revision 1.4  2001/04/30 07:17:17  eric
Movement priorities determined by bot index

Revision 1.3  2001/04/29 04:13:41  eric
Trimmed down bot structure

Revision 1.2  2001/04/29 01:36:00  eric
Added Header and Log CVS options

**********************************/

#ifndef BOT_H
#define BOT_H

// stuff for Win32 vs. Linux builds

#ifndef __linux__

typedef int (FAR *GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef void (DLLEXPORT *GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef void (FAR *LINK_ENTITY_FUNC)(entvars_t *);

#else

#include <dlfcn.h>
#define GetProcAddress dlsym

typedef int BOOL;

typedef int (*GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef void (*GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef void (*LINK_ENTITY_FUNC)(entvars_t *);

#endif


#include "ai_squad.h"


BOOL ClientConnect (edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
void ClientPutInServer (edict_t *pEntity);
void ClientCommand (edict_t *pEntity);
void FakeClientCommand (edict_t *pBot, char *arg1, char *arg2, char *arg3);

const char *Cmd_Args( void );
const char *Cmd_Argv( int argc );
int Cmd_Argc( void );

void bomb_placed (void);
void hostage_rescued (void);
void round_begin (void);

typedef enum {
	LADDER_UNKNOWN,
	LADDER_UP,
	LADDER_DOWN
} LADDER_DIRECTION;

typedef enum {
	RESPAWN_UNKNOWN,
	RESPAWN_IDLE,
	RESPAWN_NEED_TO_RESPAWN,
	RESPAWN_IS_RESPAWNING
} RESPAWN_STATE;

typedef enum {
	MSG_CS_IDLE,
	MSG_CS_TEAM_SELECT,
	MSG_CS_CT_SELECT,
	MSG_CS_T_SELECT,
	MSG_CS_BUY_MENU,
	MSG_CS_PISTOL_MENU,
	MSG_CS_SHOTGUN_MENU,
	MSG_CS_SMG_MENU,
	MSG_CS_RIFLE_MENU,
	MSG_CS_MACHINEGUN_MENU,
	MSG_CS_EQUIPMENT_MENU,
	MSG_CS_BUYING,
	MSG_CS_MENU_WAITING,
} CS_MSG;

const int BOT_SKIN_LEN = 32;
const int BOT_NAME_LEN = 32;

const int MAX_BOTS				= 32;
const int MAX_TEAMS				= 32;
const int MAX_TEAMNAME_LENGTH	= 16;

const int MAX_BOT_WHINE = 100;
const float RADIO_RANGE = 512.0;

// TODO: this may be too big of sizes (check memory on slower computers)
const float HISTORY_SAMPLING_RATE = 0.05;
const int VIEW_HISTORY_SIZE = 20;
const int VEL_HISTORY_SIZE = 60;


typedef enum
{
	ADVANCE,
	COVER,
	DEFEND,
	FLANK,
	RAPID,
	RETREAT,
	STEALTH
} MOVEMENT;

typedef enum
{
	BOMB,
	DEFUSE,
	HOSTAGE,
	RESCUE,
	ASSASINATE,
	ESCORT,
	UNKNOWN
} OBJECTIVE;

typedef struct
{
   int  iId;     // weapon ID
   int  iClip;   // amount of ammo in the clip
   int  iAmmo1;  // amount of ammo in primary reserve
   int  iAmmo2;  // amount of ammo in secondary reserve
} bot_current_weapon_t;

typedef struct
{
   // general
   bool is_used;
   RESPAWN_STATE respawn_state;
   edict_t *pEdict;
   char name[BOT_NAME_LEN+1];
   char skin[BOT_SKIN_LEN+1];
   int bot_skill; // [1,100]
   int not_started;
   CS_MSG start_action;
   bool need_to_initialize;

   // time
   float kick_time;
   float create_time;
   float spawn_time;
   float blinded_time;
   float defuse_bomb_time;
   float plant_bomb_time;

   // (written by TheFatal)
   int msecnum;
   float msecdel;
   float msecval;

   // bot state
   int bot_team;
   int bot_class;
   int bot_money;

   // buying
   char *buy_num;
   char *buy_cat;
   float buy_time;
   int buy_armor_count;
   int buy_ammo_count;
   int buy_equip_count;

   // movement
   float movement_time;
   float max_speed;
   float move_speed;
   float prev_speed;
   Vector prev_v_angle[VIEW_HISTORY_SIZE];
   Vector prev_velocity[VEL_HISTORY_SIZE];
   Vector prev_origin;

   // steering
   Vector desiredVelocity;
   Vector lookAt;

   // items
   edict_t *pickup;

   // ladders
   LADDER_DIRECTION ladder_dir;
   bool wp_ladder_top;

   // waypoints
   Vector wp_origin;
   int wp_last;
   int wp_current;
   int wp_tactical;
   float tactical_time;

   // threat
   edict_t *threat;
   Vector last_seen_threat_location;
   float last_seen_threat_time;
   float threat_giveup_time;

   // whining
   edict_t *killer_edict;
   bool  b_bot_say_killed;
   float f_bot_say_killed;

   // radio
   char *radio_type;
   char *radio_num;
   float radio_time;
   float speak_time;
   float radio_time_down;

   // combat
   float shoot_time;
   float shoot_stop_time;
   Vector aim_head;
   Vector aim_deviation;
   float aim_deviation_scale;

   // squad
   float squad_time;
   squad_t *pSquad;
   float movethink_time;
   MOVEMENT movement_style;
   SQUAD_POSITION squad_position;
   SQUAD_ROLE squad_role;
 
   // weapons
   bot_current_weapon_t current_weapon;  // one current weapon for each bot
   int m_rgAmmo[MAX_AMMO_SLOTS];  // total ammo amounts (1 array for each bot)

} bot_t;


// util.cpp
edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius );
edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue );
edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName );
edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName );
void ClientPrint( edict_t *pEdict, int msg_dest, const char *msg_name);
void UTIL_SayText( const char *pText, edict_t *pEdict );
void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message );
int UTIL_GetTeam(edict_t *pEntity);
int UTIL_GetClass(edict_t *pEntity);
int UTIL_GetBotIndex(edict_t *pEdict);
bot_t *UTIL_GetBotPointer(edict_t *pEdict);
bool IsAlive(edict_t *pEdict);
bool FInViewCone(Vector *pOrigin, edict_t *pEdict);
bool FVisible( const Vector &vecOrigin, edict_t *pEdict );
Vector Center(edict_t *pEdict);
Vector GetGunPosition(edict_t *pEdict);
void UTIL_SelectItem(edict_t *pEdict, char *item_name);
Vector VecBModelOrigin(edict_t *pEdict);
void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText );
void UTIL_BuildFileName(char *filename, char *arg1, char *arg2);


#endif // BOT_H

