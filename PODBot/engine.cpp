//
// engine.cpp
//
// Does the major work of calling the original
// Engine Functions
// 

#include "extdll.h"
#include "util.h"

#include "bot.h"
#include "bot_client.h"
#include "bot_sounds.h"
#include "waypoint.h"
#include "bot_globals.h"

void (*botMsgFunction)(void *, int) = NULL;

extern bot_t bots[MAXBOTSNUMBER];   // max of 32 bots in a game
extern Vector g_vecBomb;

int botMsgIndex;

// messages created in RegUserMsg which will be "caught"
int message_ShowMenu = 0;
int message_WeaponList = 0;
int message_CurWeapon = 0;
int message_AmmoX = 0;
int message_AmmoPickup = 0;
int message_Health = 0;
int message_Battery = 0;  // Armor
int message_Damage = 0;
int message_Money = 0;
int message_StatusIcon = 0;	// for buyzone,rescue,bombzone  
int message_SayText = 0;  
int message_MOTD = 0;  
int message_DeathMsg = 0;
int message_ScreenFade = 0;  
int message_TeamScore = 0;  

int pfnPrecacheModel(char* s)
{
   return (*g_engfuncs.pfnPrecacheModel)(s);
}
int pfnPrecacheSound(char* s)
{
   return (*g_engfuncs.pfnPrecacheSound)(s);
}
void pfnSetModel(edict_t *e, const char *m)
{
   (*g_engfuncs.pfnSetModel)(e, m);
}
int pfnModelIndex(const char *m)
{
   return (*g_engfuncs.pfnModelIndex)(m);
}
int pfnModelFrames(int modelIndex)
{
   return (*g_engfuncs.pfnModelFrames)(modelIndex);
}

void pfnSetSize(edict_t *e, const float *rgflMin, const float *rgflMax)
{
   (*g_engfuncs.pfnSetSize)(e, rgflMin, rgflMax);
}
void pfnChangeLevel(char* s1, char* s2)
{
   // kick any bot off of the server after time/frag limit...
   for (int index = 0; index <MAXBOTSNUMBER; index++)
   {
      if (bots[index].is_used)  // is this slot used?
      {
         char cmd[40];

         sprintf(cmd, "kick \"%s\"\n", bots[index].name);
         SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
         bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
      }
   }

   // Save collected Experience on Map Change
   SaveExperienceTab(NULL);
   (*g_engfuncs.pfnChangeLevel)(s1, s2);
}
void pfnGetSpawnParms(edict_t *ent)
{
   (*g_engfuncs.pfnGetSpawnParms)(ent);
}
void pfnSaveSpawnParms(edict_t *ent)
{
   (*g_engfuncs.pfnSaveSpawnParms)(ent);
}
float pfnVecToYaw(const float *rgflVector)
{
   return (*g_engfuncs.pfnVecToYaw)(rgflVector);
}
void pfnVecToAngles(const float *rgflVectorIn, float *rgflVectorOut)
{
   (*g_engfuncs.pfnVecToAngles)(rgflVectorIn, rgflVectorOut);
}
void pfnMoveToOrigin(edict_t *ent, const float *pflGoal, float dist, int iMoveType)
{
   (*g_engfuncs.pfnMoveToOrigin)(ent, pflGoal, dist, iMoveType);
}

void pfnChangeYaw(edict_t* ent)
{
   (*g_engfuncs.pfnChangeYaw)(ent);
}

void pfnChangePitch(edict_t* ent)
{
   (*g_engfuncs.pfnChangePitch)(ent);
}
edict_t* pfnFindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue)
{
	if(FStrEq(pszValue, "func_bomb_target"))
	{
		// Clear the Array of Bots
		int i;
		bot_t *pBot;
		for(i=0;i<MAXBOTSNUMBER;i++)
		{
			pBot = &bots[i];
			if(pBot->is_used==TRUE)
			{
				pBot->need_to_initialize = TRUE;
			}
			iRadioSelect[i]=0;
		}

		g_bBombPlanted = FALSE;
		g_bBombSayString = FALSE;
		g_fTimeBombPlanted = 0.0;
		g_vecBomb = Vector(0,0,0);
		// Clear Waypoint Indices of visited Bomb Spots
		for(i=0;i<MAXNUMBOMBSPOTS;i++)
			g_rgiBombSpotsVisited[i] = -1;
		g_iLastBombPoint = -1;
		g_fTimeNextBombUpdate = 0.0;
		
		g_bLeaderChosenT = FALSE;
		g_bLeaderChosenCT = FALSE;

		g_bHostageRescued = FALSE;
		g_rgfLastRadioTime[0] = 0.0;
		g_rgfLastRadioTime[1] = 0.0;
		g_bBotsCanPause = FALSE;

		// Clear Array of Player Stats
		for (i = 0; i < gpGlobals->maxClients;i++)
		{
			ThreatTab[i].vecSoundPosition = Vector(0,0,0);
			ThreatTab[i].fHearingDistance = 0.0;
			ThreatTab[i].fTimeSoundLasting = 0.0;
		}

		// Update Experience Data on Round Start
		UpdateGlobalExperienceData();
        SaveExperienceTab(NULL);
		// Calculate the Round Mid/End in World Time
		g_fTimeRoundEnd = CVAR_GET_FLOAT("mp_roundtime");
		g_fTimeRoundEnd*=60;
		g_fTimeRoundEnd+= CVAR_GET_FLOAT("mp_freezetime");
		g_fTimeRoundMid = g_fTimeRoundEnd/2;
		g_fTimeRoundEnd+=gpGlobals->time;
		g_fTimeRoundMid+=gpGlobals->time;
	}
	return (*g_engfuncs.pfnFindEntityByString)(pEdictStartSearchAfter, pszField, pszValue);
}


int pfnGetEntityIllum(edict_t* pEnt)
{
   return (*g_engfuncs.pfnGetEntityIllum)(pEnt);
}
edict_t* pfnFindEntityInSphere(edict_t *pEdictStartSearchAfter, const float *org, float rad)
{
   return (*g_engfuncs.pfnFindEntityInSphere)(pEdictStartSearchAfter, org, rad);
}
edict_t* pfnFindClientInPVS(edict_t *pEdict)
{
   return (*g_engfuncs.pfnFindClientInPVS)(pEdict);
}
edict_t* pfnEntitiesInPVS(edict_t *pplayer)
{
   return (*g_engfuncs.pfnEntitiesInPVS)(pplayer);
}
void pfnMakeVectors(const float *rgflVector)
{
   (*g_engfuncs.pfnMakeVectors)(rgflVector);
}
void pfnAngleVectors(const float *rgflVector, float *forward, float *right, float *up)
{
   (*g_engfuncs.pfnAngleVectors)(rgflVector, forward, right, up);
}
edict_t* pfnCreateEntity(void)
{
   return (*g_engfuncs.pfnCreateEntity)();
}
void pfnRemoveEntity(edict_t* e)
{
   (*g_engfuncs.pfnRemoveEntity)(e);
}
edict_t* pfnCreateNamedEntity(int className)
{
   return (*g_engfuncs.pfnCreateNamedEntity)(className);
}
void pfnMakeStatic(edict_t *ent)
{
   (*g_engfuncs.pfnMakeStatic)(ent);
}
int pfnEntIsOnFloor(edict_t *e)
{
   return (*g_engfuncs.pfnEntIsOnFloor)(e);
}
int pfnDropToFloor(edict_t* e)
{
   return (*g_engfuncs.pfnDropToFloor)(e);
}
int pfnWalkMove(edict_t *ent, float yaw, float dist, int iMode)
{
   return (*g_engfuncs.pfnWalkMove)(ent, yaw, dist, iMode);
}
void pfnSetOrigin(edict_t *e, const float *rgflOrigin)
{
   (*g_engfuncs.pfnSetOrigin)(e, rgflOrigin);
}
void pfnEmitSound(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch)
{
	SoundAttachToThreat(entity,sample,volume);
   (*g_engfuncs.pfnEmitSound)(entity, channel, sample, volume, attenuation, fFlags, pitch);
}
void pfnEmitAmbientSound(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch)
{
   (*g_engfuncs.pfnEmitAmbientSound)(entity, pos, samp, vol, attenuation, fFlags, pitch);
}
void pfnTraceLine(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceLine)(v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceToss(edict_t* pent, edict_t* pentToIgnore, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceToss)(pent, pentToIgnore, ptr);
}
int pfnTraceMonsterHull(edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
   return (*g_engfuncs.pfnTraceMonsterHull)(pEdict, v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceHull(const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceHull)(v1, v2, fNoMonsters, hullNumber, pentToSkip, ptr);
}
void pfnTraceModel(const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceModel)(v1, v2, hullNumber, pent, ptr);
}
const char *pfnTraceTexture(edict_t *pTextureEntity, const float *v1, const float *v2 )
{
   return (*g_engfuncs.pfnTraceTexture)(pTextureEntity, v1, v2);
}
void pfnTraceSphere(const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr)
{
   (*g_engfuncs.pfnTraceSphere)(v1, v2, fNoMonsters, radius, pentToSkip, ptr);
}
void pfnGetAimVector(edict_t* ent, float speed, float *rgflReturn)
{
   (*g_engfuncs.pfnGetAimVector)(ent, speed, rgflReturn);
}
void pfnServerCommand(char* str)
{
   (*g_engfuncs.pfnServerCommand)(str);
}
void pfnServerExecute(void)
{
   (*g_engfuncs.pfnServerExecute)();
}
void pfnClientCommand(edict_t* pEdict, char* szFmt, ...)
{
   return;
}

void pfnParticleEffect(const float *org, const float *dir, float color, float count)
{
   (*g_engfuncs.pfnParticleEffect)(org, dir, color, count);
}
void pfnLightStyle(int style, char* val)
{
   (*g_engfuncs.pfnLightStyle)(style, val);
}
int pfnDecalIndex(const char *name)
{
   return (*g_engfuncs.pfnDecalIndex)(name);
}
int pfnPointContents(const float *rgflVector)
{
   return (*g_engfuncs.pfnPointContents)(rgflVector);
}

// Called each Time a Message is about to sent 
void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
	state=0;
	if (gpGlobals->deathmatch)
	{
		int index = -1;
		
		// Bot involved ?
		if (ed && FBitSet(ed->v.flags, FL_FAKECLIENT))
		{
			index = UTIL_GetBotIndex(ed);
			//		 iMessageLoopIndex=index;
			
			// is this message for a bot?
			if (index != -1)
			{
				botMsgFunction = NULL;  // no msg function until known otherwise
				botMsgIndex = index;    // index of bot receiving message

				// Message handling is done in bot_client.cpp
				if (msg_type == message_ShowMenu)
					botMsgFunction = BotClient_CS_ShowMenu;
				else if (msg_type == message_WeaponList)
					botMsgFunction = BotClient_CS_WeaponList;
				else if (msg_type == message_CurWeapon)
					botMsgFunction = BotClient_CS_CurrentWeapon;
				else if (msg_type == message_AmmoX)
					botMsgFunction = BotClient_CS_AmmoX;
				else if (msg_type == message_AmmoPickup)
					botMsgFunction = BotClient_CS_AmmoPickup;
				else if (msg_type == message_Health)
					botMsgFunction = BotClient_CS_Health;
				else if (msg_type == message_Battery)
					botMsgFunction = BotClient_CS_Battery;
				else if (msg_type == message_Damage)
					botMsgFunction = BotClient_CS_Damage;
				else if (msg_type == message_Money)
					botMsgFunction = BotClient_CS_Money;
				else if (msg_type == message_StatusIcon)
					botMsgFunction = BotClient_CS_StatusIcon;
				else if (msg_type == message_ScreenFade)
					botMsgFunction = BotClient_CS_ScreenFade;
				else if (msg_type == message_SayText)
					botMsgFunction = BotClient_CS_SayText;
				else if (msg_type == message_MOTD)
					botMsgFunction = BotClient_CS_MOTD;
/*				else if (msg_type == message_TeamScore)
					botMsgFunction = BotClient_CS_TeamScore;*/
			}

			
		}
		// Show the Waypoint copyright Message right at the MOTD
		else if(msg_type == message_MOTD || msg_type == message_ShowMenu)
		{
			g_bMapInitialised = TRUE;
			g_bNeedHUDDisplay = TRUE;
		}
		else if (msg_dest == MSG_ALL)
		{
			botMsgFunction = NULL;  // no msg function until known otherwise
			botMsgIndex = -1;       // index of bot receiving message (none)
			
			if (msg_type == message_DeathMsg)
				botMsgFunction = BotClient_CS_DeathMsg;
			else if(msg_type == SVC_INTERMISSION)
			{
				bot_t *pBot;
				
				for(int i=0;i<MAXBOTSNUMBER;i++)
				{
					pBot = &bots[i];
					if((pBot->is_used == TRUE))
						pBot->bDead = TRUE;
				}
			}
		}
		
	}
	
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}

void pfnMessageEnd(void)
{
	if (gpGlobals->deathmatch)
	{
		state=0;
		botMsgFunction = NULL;
	}
	
	(*g_engfuncs.pfnMessageEnd)();
}

void pfnWriteByte(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteByte)(iValue);
}

void pfnWriteChar(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteChar)(iValue);
}

void pfnWriteShort(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteShort)(iValue);
}

void pfnWriteLong(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteLong)(iValue);
}

void pfnWriteAngle(float flValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&flValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteAngle)(flValue);
}

void pfnWriteCoord(float flValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&flValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteCoord)(flValue);
}

void pfnWriteString(const char *sz)
{
	if (gpGlobals->deathmatch)
	{
	// Check if it's the "Bomb Planted" Message
		if (FStrEq(sz,"%!MRAD_BOMBPL"))
		{
			if(!g_bBombPlanted)
			{
				g_bBombPlanted = g_bBombSayString = TRUE;
				g_fTimeBombPlanted = gpGlobals->time;
			}
		}
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)((void *)sz, botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteString)(sz);
}

void pfnWriteEntity(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteEntity)(iValue);
}

void pfnCVarRegister(cvar_t *pCvar)
{
   (*g_engfuncs.pfnCVarRegister)(pCvar);
}
float pfnCVarGetFloat(const char *szVarName)
{
   return (*g_engfuncs.pfnCVarGetFloat)(szVarName);
}
const char* pfnCVarGetString(const char *szVarName)
{
   return (*g_engfuncs.pfnCVarGetString)(szVarName);
}
void pfnCVarSetFloat(const char *szVarName, float flValue)
{
   (*g_engfuncs.pfnCVarSetFloat)(szVarName, flValue);
}
void pfnCVarSetString(const char *szVarName, const char *szValue)
{
   (*g_engfuncs.pfnCVarSetString)(szVarName, szValue);
}
void* pfnPvAllocEntPrivateData(edict_t *pEdict, long cb)
{
   return (*g_engfuncs.pfnPvAllocEntPrivateData)(pEdict, cb);
}
void* pfnPvEntPrivateData(edict_t *pEdict)
{
   return (*g_engfuncs.pfnPvEntPrivateData)(pEdict);
}
void pfnFreeEntPrivateData(edict_t *pEdict)
{
   (*g_engfuncs.pfnFreeEntPrivateData)(pEdict);
}
const char* pfnSzFromIndex(int iString)
{
   return (*g_engfuncs.pfnSzFromIndex)(iString);
}
int pfnAllocString(const char *szValue)
{
   return (*g_engfuncs.pfnAllocString)(szValue);
}
edict_t* pfnPEntityOfEntOffset(int iEntOffset)
{
   return (*g_engfuncs.pfnPEntityOfEntOffset)(iEntOffset);
}
int pfnEntOffsetOfPEntity(const edict_t *pEdict)
{
   return (*g_engfuncs.pfnEntOffsetOfPEntity)(pEdict);
}
int pfnIndexOfEdict(const edict_t *pEdict)
{
   return (*g_engfuncs.pfnIndexOfEdict)(pEdict);
}
edict_t* pfnPEntityOfEntIndex(int iEntIndex)
{
   return (*g_engfuncs.pfnPEntityOfEntIndex)(iEntIndex);
}
void* pfnGetModelPtr(edict_t* pEdict)
{
   return (*g_engfuncs.pfnGetModelPtr)(pEdict);
}

// Sends the Index of a Message
int pfnRegUserMsg(const char *pszName, int iSize)
{
   int msg;

   msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

   if (gpGlobals->deathmatch)
   {
         if (strcmp(pszName, "ShowMenu") == 0)
            message_ShowMenu = msg;
         else if (strcmp(pszName, "WeaponList") == 0)
            message_WeaponList = msg;
         else if (strcmp(pszName, "CurWeapon") == 0)
            message_CurWeapon = msg;
         else if (strcmp(pszName, "AmmoX") == 0)
            message_AmmoX = msg;
         else if (strcmp(pszName, "AmmoPickup") == 0)
            message_AmmoPickup = msg;
         else if (strcmp(pszName, "Health") == 0)
            message_Health = msg;
         else if (strcmp(pszName, "Battery") == 0)
            message_Battery = msg;
         else if (strcmp(pszName, "Damage") == 0)
            message_Damage = msg;
         else if (strcmp(pszName, "Money") == 0)
            message_Money = msg;
         else if (strcmp(pszName, "StatusIcon") == 0)
            message_StatusIcon = msg;
		 else if(strcmp(pszName, "SayText") == 0)
            message_SayText = msg;
		 else if(strcmp(pszName, "MOTD") == 0)
            message_MOTD = msg;
         else if (strcmp(pszName, "DeathMsg") == 0)
            message_DeathMsg = msg;
		 else if(strcmp(pszName, "ScreenFade") == 0)
            message_ScreenFade = msg;
		 else if(strcmp(pszName, "TeamScore") == 0)
            message_TeamScore = msg;
   }

   return msg;
}

void pfnAnimationAutomove(const edict_t* pEdict, float flTime)
{
   (*g_engfuncs.pfnAnimationAutomove)(pEdict, flTime);
}
void pfnGetBonePosition(const edict_t* pEdict, int iBone, float *rgflOrigin, float *rgflAngles )
{
   (*g_engfuncs.pfnGetBonePosition)(pEdict, iBone, rgflOrigin, rgflAngles);
}
unsigned long pfnFunctionFromName( const char *pName )
{
   return (*g_engfuncs.pfnFunctionFromName)(pName);
}
const char *pfnNameForFunction( unsigned long function )
{
   return (*g_engfuncs.pfnNameForFunction)(function);
}
void pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg )
{
   (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
}
void pfnServerPrint( const char *szMsg )
{
   (*g_engfuncs.pfnServerPrint)(szMsg);
}
void pfnGetAttachment(const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles )
{
   (*g_engfuncs.pfnGetAttachment)(pEdict, iAttachment, rgflOrigin, rgflAngles);
}
void pfnCRC32_Init(CRC32_t *pulCRC)
{
   (*g_engfuncs.pfnCRC32_Init)(pulCRC);
}
void pfnCRC32_ProcessBuffer(CRC32_t *pulCRC, void *p, int len)
{
   (*g_engfuncs.pfnCRC32_ProcessBuffer)(pulCRC, p, len);
}
void pfnCRC32_ProcessByte(CRC32_t *pulCRC, unsigned char ch)
{
   (*g_engfuncs.pfnCRC32_ProcessByte)(pulCRC, ch);
}
CRC32_t pfnCRC32_Final(CRC32_t pulCRC)
{
   return (*g_engfuncs.pfnCRC32_Final)(pulCRC);
}
long pfnRandomLong(long lLow, long lHigh)
{
   return (*g_engfuncs.pfnRandomLong)(lLow, lHigh);
}
float pfnRandomFloat(float flLow, float flHigh)
{
   return (*g_engfuncs.pfnRandomFloat)(flLow, flHigh);
}
void pfnSetView(const edict_t *pClient, const edict_t *pViewent )
{
   (*g_engfuncs.pfnSetView)(pClient, pViewent);
}
float pfnTime( void )
{
   return (*g_engfuncs.pfnTime)();
}
void pfnCrosshairAngle(const edict_t *pClient, float pitch, float yaw)
{
   (*g_engfuncs.pfnCrosshairAngle)(pClient, pitch, yaw);
}
byte *pfnLoadFileForMe(char *filename, int *pLength)
{
   return (*g_engfuncs.pfnLoadFileForMe)(filename, pLength);
}
void pfnFreeFile(void *buffer)
{
   (*g_engfuncs.pfnFreeFile)(buffer);
}
void pfnEndSection(const char *pszSectionName)
{
   (*g_engfuncs.pfnEndSection)(pszSectionName);
}
int pfnCompareFileTime(char *filename1, char *filename2, int *iCompare)
{
   return (*g_engfuncs.pfnCompareFileTime)(filename1, filename2, iCompare);
}
void pfnGetGameDir(char *szGetGameDir)
{
   (*g_engfuncs.pfnGetGameDir)(szGetGameDir);
}
void pfnCvar_RegisterVariable(cvar_t *variable)
{
   (*g_engfuncs.pfnCvar_RegisterVariable)(variable);
}
void pfnFadeClientVolume(const edict_t *pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds)
{
   (*g_engfuncs.pfnFadeClientVolume)(pEdict, fadePercent, fadeOutSeconds, holdTime, fadeInSeconds);
}
void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed)
{
	bot_t *pBot=UTIL_GetBotPointer((edict_t*)pEdict);
	if(pBot!=NULL)
		pBot->f_max_speed=fNewMaxspeed;
   (*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
}
edict_t * pfnCreateFakeClient(const char *netname)
{
   return (*g_engfuncs.pfnCreateFakeClient)(netname);
}
void pfnRunPlayerMove(edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec )
{
   (*g_engfuncs.pfnRunPlayerMove)(fakeclient, viewangles, forwardmove, sidemove, upmove, buttons, impulse, msec);
}
int pfnNumberOfEntities(void)
{
   return (*g_engfuncs.pfnNumberOfEntities)();
}
char* pfnGetInfoKeyBuffer(edict_t *e)
{
   return (*g_engfuncs.pfnGetInfoKeyBuffer)(e);
}
char* pfnInfoKeyValue(char *infobuffer, char *key)
{
   return (*g_engfuncs.pfnInfoKeyValue)(infobuffer, key);
}
void pfnSetKeyValue(char *infobuffer, char *key, char *value)
{
   (*g_engfuncs.pfnSetKeyValue)(infobuffer, key, value);
}
void pfnSetClientKeyValue(int clientIndex, char *infobuffer, char *key, char *value)
{
   (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
}
int pfnIsMapValid(char *filename)
{
   return (*g_engfuncs.pfnIsMapValid)(filename);
}
void pfnStaticDecal( const float *origin, int decalIndex, int entityIndex, int modelIndex )
{
   (*g_engfuncs.pfnStaticDecal)(origin, decalIndex, entityIndex, modelIndex);
}
int pfnPrecacheGeneric(char* s)
{
   return (*g_engfuncs.pfnPrecacheGeneric)(s);
}
int pfnGetPlayerUserId(edict_t *e )
{
   return (*g_engfuncs.pfnGetPlayerUserId)(e);
}
void pfnBuildSoundMsg(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
   (*g_engfuncs.pfnBuildSoundMsg)(entity, channel, sample, volume, attenuation, fFlags, pitch, msg_dest, msg_type, pOrigin, ed);
}
int pfnIsDedicatedServer(void)
{
   return (*g_engfuncs.pfnIsDedicatedServer)();
}
