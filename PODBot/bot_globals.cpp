#include "bot_globals.h"
#include "extdll.h"

// Type of Map - Assault,Defuse etc... 
int g_iMapType;	
bool g_bWaypointOn = FALSE;
bool g_bWaypointsChanged = TRUE;
bool g_bAutoWaypoint = FALSE;
bool g_bPathWaypoint = FALSE;
bool g_bDangerDirection = FALSE;
bool g_bLearnJumpWaypoint = FALSE;
int g_iFindWPIndex = -1;
int g_iCacheWaypointIndex = -1;
// number of waypoints currently in use for each team
int g_iNumWaypoints;

// Leader for both Teams chosen
bool g_bLeaderChosenT = FALSE;
bool g_bLeaderChosenCT = FALSE;

int g_rgiTerrorWaypoints[MAX_WAYPOINTS];
int g_iNumTerrorPoints = 0;
int g_rgiCTWaypoints[MAX_WAYPOINTS];
int g_iNumCTPoints = 0;
int g_rgiGoalWaypoints[MAX_WAYPOINTS];
int g_iNumGoalPoints = 0;
int g_rgiCampWaypoints[MAX_WAYPOINTS];
int g_iNumCampPoints = 0;
// time that this waypoint was displayed (while editing)
float g_rgfWPDisplayTime[MAX_WAYPOINTS];
// declare the array of head pointers to the path structures...
int *g_pFloydDistanceMatrix = NULL;

bool g_bMapInitialised = FALSE;
bool g_bRecalcVis = TRUE;
int g_iCurrVisIndex = 0;
unsigned char g_rgbyVisLUT[MAX_WAYPOINTS][MAX_WAYPOINTS/4];

char g_cKillHistory;

// Default Tables for Personality Weapon Prefs overridden by
// botweapons.cfg
int NormalWeaponPrefs[NUM_WEAPONS]={
   0,1,2,4,3,5,6,15,8,10,9,7,18,16,17,11,12,13,14};
int AgressiveWeaponPrefs[NUM_WEAPONS]={
   0,1,2,4,3,15,16,17,8,10,9,7,5,6,13,12,14,11,18};
int DefensiveWeaponPrefs[NUM_WEAPONS]={
   0,1,2,4,3,5,6,8,10,9,7,18,11,13,12,14,15,17,16};

int* ptrWeaponPrefs[]={
	(int*)&NormalWeaponPrefs,(int*)&AgressiveWeaponPrefs,(int*)&DefensiveWeaponPrefs};

int iNumBotnames;
int iNumKillChats;
int iNumBombChats;
int iNumDeadChats;
int iNumNoKeyStrings;


int min_bots = -1;
int max_bots = -1;
float botcreation_time = 0.0;
// Signals if still busy in creating a Bot
bool g_bBusyCreatingBot = FALSE;
// When creating Bots with random Skill, Skill Numbers are assigned
// between these 2 values
int g_iMinBotSkill = 0;
int g_iMaxBotSkill = 100;
// Flag for Botchatting
bool g_bBotChat = TRUE;
// Flag for Jasonmode (Knife only)
bool g_bJasonMode = FALSE;
// Switches Skilldisplay on/off in Botnames
bool g_bDetailNames = TRUE;
// Toggles inhuman turning on/off
bool g_bInstantTurns=FALSE;
// Maximum Number of Bots to follow User
int g_iMaxNumFollow = 3;
// Stores the maximum number of Weapons
// Bots are allowed to pickup
int g_iMaxWeaponPickup = 1;
// Stores if Bots are allowed to pierce thru Walls
bool g_bShootThruWalls = TRUE;
// Stores Last Time chatted - prevents spamming
float g_fLastChatTime = 0.0;
// Stores the End of the round (in worldtime)
// gpGlobals->time+roundtime
float g_fTimeRoundEnd = 0.0;
// Stores the halftime of the round (in worldtime)
float g_fTimeRoundMid = 0.0;
// These Variables keep the Update Time Offset 
float g_fTimeSoundUpdate = 1.0;
float g_fTimePickupUpdate = 0.3;
float g_fTimeGrenadeUpdate = 0.5;
// Holds the time to allow the next Search Update
float g_fTimeNextBombUpdate = 0.0;
// Stores the last checked Bomb Waypoint
int g_iLastBombPoint;
// Stores if the Bomb was planted
bool g_bBombPlanted;
bool g_bBombSayString;
// Holds the time when Bomb was planted
float g_fTimeBombPlanted = 0.0;
// Stores visited Bombspots for Counters when
// Bomb has been planted 
int g_rgiBombSpotsVisited[MAXNUMBOMBSPOTS];
bool g_bUseSpeech=TRUE;
// Allows Bots voting against players 
bool g_bAllowVotes=TRUE;
bool g_bUseExperience = TRUE;
// Bot Spraying on/off
bool g_bBotSpray = TRUE;
// Stores if Bots should pause
bool g_bBotsCanPause = FALSE; 
// Stores if Counter rescued a Hostage in a Round
// Needed for T Bots to know if they should take away
// hostages themselves
bool g_bHostageRescued = FALSE;
int iRadioSelect[32];
int g_rgfLastRadio[2];
// Stores Time of RadioMessage - prevents too fast responds
float g_rgfLastRadioTime[2] = {0.0,0.0};

bool g_bNeedHUDDisplay = TRUE;
bool g_bCreatorShown = FALSE;
float g_fTimeHUDDisplay;
// String displayed to Host telling about Waypoints
char g_szWaypointMessage[512];

// Stores the current FunMode (0 disabled)
char g_cFunMode = 0;
float g_fTimeFunUpdate = 0.0;
// Number of available Spraypaints
int g_iNumLogos=5;
// Default Spaynames - overridden by BotLogos.cfg
char szSprayNames[NUM_SPRAYPAINTS][20] = {
	"{biohaz",
	"{graf004",
	"{graf005",
	"{lambda06",
	"{target",
	"{hand1"};


int state;

// Assigns a goal in debug mode
int g_iDebugGoalIndex = -1;

