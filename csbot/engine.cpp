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

$Source: /usr/local/cvsroot/csbot/src/dlls/engine.cpp,v $
$Revision: 1.5 $
$Date: 2001/04/30 08:06:11 $
$State: Exp $

***********************************

$Log: engine.cpp,v $
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

#include "bot.h"
#include "bot_client.h"
#include "engine.h"


extern enginefuncs_t g_engfuncs;
extern bot_t bots[MAX_BOTS];


int debug_engine = 0;

void (*botMsgFunction)(void *, int) = NULL;
void (*botMsgEndFunction)(void *, int) = NULL;
int botMsgIndex;

// messages created in RegUserMsg which will be "caught"
int message_ShowMenu = 0;
int message_WeaponList = 0;
int message_CurWeapon = 0;
int message_AmmoX = 0;
int message_AmmoPickup = 0;
int message_Damage = 0;
int message_Money = 0;  // for Counter-Strike
int message_DeathMsg = 0;
int message_TextMsg = 0;
int message_ScreenFade = 0;

static FILE *fp;


int pfnPrecacheModel(char* s)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPrecacheModel: %s\n",s); fclose(fp); }
   return (*g_engfuncs.pfnPrecacheModel)(s);
}
int pfnPrecacheSound(char* s)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPrecacheSound: %s\n",s); fclose(fp); }
   return (*g_engfuncs.pfnPrecacheSound)(s);
}
void pfnSetModel(edict_t *e, const char *m)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSetModel: edict=%x %s\n",e,m); fclose(fp); }
   (*g_engfuncs.pfnSetModel)(e, m);
}
int pfnModelIndex(const char *m)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnModelIndex: %s\n",m); fclose(fp); }
   return (*g_engfuncs.pfnModelIndex)(m);
}
int pfnModelFrames(int modelIndex)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnModelFrames:\n"); fclose(fp); }
   return (*g_engfuncs.pfnModelFrames)(modelIndex);
}
void pfnSetSize(edict_t *e, const float *rgflMin, const float *rgflMax)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSetSize: %x\n",e); fclose(fp); }
   (*g_engfuncs.pfnSetSize)(e, rgflMin, rgflMax);
}
void pfnChangeLevel(char* s1, char* s2)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnChangeLevel:\n"); fclose(fp); }

   // kick any bot off of the server after time/frag limit...
   for (int index = 0; index < MAX_BOTS; index++)
   {
      if (bots[index].is_used)  // is this slot used?
      {
         char cmd[40];

         sprintf(cmd, "kick \"%s\"\n", bots[index].name);

         bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;

         SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
      }
   }

   (*g_engfuncs.pfnChangeLevel)(s1, s2);
}
void pfnGetSpawnParms(edict_t *ent)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetSpawnParms:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetSpawnParms)(ent);
}
void pfnSaveSpawnParms(edict_t *ent)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSaveSpawnParms:\n"); fclose(fp); }
   (*g_engfuncs.pfnSaveSpawnParms)(ent);
}
float pfnVecToYaw(const float *rgflVector)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnVecToYaw:\n"); fclose(fp); }
   return (*g_engfuncs.pfnVecToYaw)(rgflVector);
}
void pfnVecToAngles(const float *rgflVectorIn, float *rgflVectorOut)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnVecToAngles:\n"); fclose(fp); }
   (*g_engfuncs.pfnVecToAngles)(rgflVectorIn, rgflVectorOut);
}
void pfnMoveToOrigin(edict_t *ent, const float *pflGoal, float dist, int iMoveType)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnMoveToOrigin:\n"); fclose(fp); }
   (*g_engfuncs.pfnMoveToOrigin)(ent, pflGoal, dist, iMoveType);
}
void pfnChangeYaw(edict_t* ent)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnChangeYaw:\n"); fclose(fp); }
   (*g_engfuncs.pfnChangeYaw)(ent);
}
void pfnChangePitch(edict_t* ent)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnChangePitch:\n"); fclose(fp); }
   (*g_engfuncs.pfnChangePitch)(ent);
}
/*
edict_t* pfnFindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFindEntityByString: %s\n",pszValue); fclose(fp); }
   return (*g_engfuncs.pfnFindEntityByString)(pEdictStartSearchAfter, pszField, pszValue);
}*/
edict_t* pfnFindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue)
{
	if (strcmp (pszValue, "func_bomb_target") == 0)
		round_begin ();
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFindEntityByString: %s\n",pszValue); fclose(fp); }
	return (*g_engfuncs.pfnFindEntityByString)(pEdictStartSearchAfter, pszField, pszValue);
}
int pfnGetEntityIllum(edict_t* pEnt)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetEntityIllum:\n"); fclose(fp); }
   return (*g_engfuncs.pfnGetEntityIllum)(pEnt);
}
edict_t* pfnFindEntityInSphere(edict_t *pEdictStartSearchAfter, const float *org, float rad)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFindEntityInSphere:\n"); fclose(fp); }
   return (*g_engfuncs.pfnFindEntityInSphere)(pEdictStartSearchAfter, org, rad);
}
edict_t* pfnFindClientInPVS(edict_t *pEdict)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFindClientInPVS:\n"); fclose(fp); }
   return (*g_engfuncs.pfnFindClientInPVS)(pEdict);
}
edict_t* pfnEntitiesInPVS(edict_t *pplayer)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnEntitiesInPVS:\n"); fclose(fp); }
   return (*g_engfuncs.pfnEntitiesInPVS)(pplayer);
}
void pfnMakeVectors(const float *rgflVector)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnMakeVectors:\n"); fclose(fp); }
   (*g_engfuncs.pfnMakeVectors)(rgflVector);
}
void pfnAngleVectors(const float *rgflVector, float *forward, float *right, float *up)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnAngleVectors:\n"); fclose(fp); }
   (*g_engfuncs.pfnAngleVectors)(rgflVector, forward, right, up);
}
edict_t* pfnCreateEntity(void)
{
   edict_t *pent = (*g_engfuncs.pfnCreateEntity)();
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCreateEntity: %x\n",pent); fclose(fp); }
   return pent;
}
void pfnRemoveEntity(edict_t* e)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnRemoveEntity: %x\n",e); fclose(fp); }
   if (debug_engine)
   {
      fp=fopen("bot.txt","a");
      fprintf(fp,"pfnRemoveEntity: %x\n",e);
      if (e->v.model != 0)
         fprintf(fp," model=%s\n", STRING(e->v.model));
      fclose(fp);
   }

   (*g_engfuncs.pfnRemoveEntity)(e);
}
edict_t* pfnCreateNamedEntity(int className)
{
   edict_t *pent = (*g_engfuncs.pfnCreateNamedEntity)(className);
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCreateNamedEntity: edict=%x name=%s\n",pent,STRING(className)); fclose(fp); }
   return pent;
}
void pfnMakeStatic(edict_t *ent)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnMakeStatic:\n"); fclose(fp); }
   (*g_engfuncs.pfnMakeStatic)(ent);
}
int pfnEntIsOnFloor(edict_t *e)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnEntIsOnFloor:\n"); fclose(fp); }
   return (*g_engfuncs.pfnEntIsOnFloor)(e);
}
int pfnDropToFloor(edict_t* e)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnDropToFloor:\n"); fclose(fp); }
   return (*g_engfuncs.pfnDropToFloor)(e);
}
int pfnWalkMove(edict_t *ent, float yaw, float dist, int iMode)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWalkMove:\n"); fclose(fp); }
   return (*g_engfuncs.pfnWalkMove)(ent, yaw, dist, iMode);
}
void pfnSetOrigin(edict_t *e, const float *rgflOrigin)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSetOrigin:\n"); fclose(fp); }
   (*g_engfuncs.pfnSetOrigin)(e, rgflOrigin);
}
void pfnEmitSound(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnEmitSound:\n"); fclose(fp); }
   (*g_engfuncs.pfnEmitSound)(entity, channel, sample, volume, attenuation, fFlags, pitch);
}
void pfnEmitAmbientSound(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnEmitAmbientSound:\n"); fclose(fp); }
   (*g_engfuncs.pfnEmitAmbientSound)(entity, pos, samp, vol, attenuation, fFlags, pitch);
}
void pfnTraceLine(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTraceLine:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceLine)(v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceToss(edict_t* pent, edict_t* pentToIgnore, TraceResult *ptr)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTraceToss:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceToss)(pent, pentToIgnore, ptr);
}
int pfnTraceMonsterHull(edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTraceMonsterHull:\n"); fclose(fp); }
   return (*g_engfuncs.pfnTraceMonsterHull)(pEdict, v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceHull(const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTraceHull:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceHull)(v1, v2, fNoMonsters, hullNumber, pentToSkip, ptr);
}
void pfnTraceModel(const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTraceModel:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceModel)(v1, v2, hullNumber, pent, ptr);
}
const char *pfnTraceTexture(edict_t *pTextureEntity, const float *v1, const float *v2 )
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTraceTexture:\n"); fclose(fp); }
   return (*g_engfuncs.pfnTraceTexture)(pTextureEntity, v1, v2);
}
void pfnTraceSphere(const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTraceSphere:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceSphere)(v1, v2, fNoMonsters, radius, pentToSkip, ptr);
}
void pfnGetAimVector(edict_t* ent, float speed, float *rgflReturn)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetAimVector:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetAimVector)(ent, speed, rgflReturn);
}
void pfnServerCommand(char* str)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnServerCommand: %s\n",str); fclose(fp); }
   (*g_engfuncs.pfnServerCommand)(str);
}
void pfnServerExecute(void)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnServerExecute:\n"); fclose(fp); }
   (*g_engfuncs.pfnServerExecute)();
}
void pfnClientCommand(edict_t* pEdict, char* szFmt, ...)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnClientCommand=%s\n",szFmt); fclose(fp); }
   return;
}
void pfnParticleEffect(const float *org, const float *dir, float color, float count)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnParticleEffect:\n"); fclose(fp); }
   (*g_engfuncs.pfnParticleEffect)(org, dir, color, count);
}
void pfnLightStyle(int style, char* val)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnLightStyle:\n"); fclose(fp); }
   (*g_engfuncs.pfnLightStyle)(style, val);
}
int pfnDecalIndex(const char *name)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnDecalIndex:\n"); fclose(fp); }
   return (*g_engfuncs.pfnDecalIndex)(name);
}
int pfnPointContents(const float *rgflVector)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPointContents:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPointContents)(rgflVector);
}
void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
   if (gpGlobals->deathmatch)
   {
      int index = -1;

      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnMessageBegin: edict=%x dest=%d type=%d\n",ed,msg_dest,msg_type); fclose(fp); }

      if (ed)
      {
         index = UTIL_GetBotIndex(ed);

         // is this message for a bot?
         if (index != -1)
         {
            botMsgFunction = NULL;     // no msg function until known otherwise
            botMsgEndFunction = NULL;  // no msg end function until known otherwise
            botMsgIndex = index;       // index of bot receiving message

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
               else if (msg_type == message_Damage)
                  botMsgFunction = BotClient_CS_Damage;
               else if (msg_type == message_Money)
                  botMsgFunction = BotClient_CS_Money;
               else if (msg_type == message_ScreenFade)
                  botMsgFunction = BotClient_CS_ScreenFade;
			   else if (msg_type == message_TextMsg)
                  botMsgFunction = BotClient_CS_TextMsg;
         }
      }
      else if (msg_dest == MSG_ALL)
      {
         botMsgFunction = NULL;  // no msg function until known otherwise
         botMsgIndex = -1;       // index of bot receiving message (none)

         if (msg_type == message_DeathMsg)
            botMsgFunction = BotClient_CS_DeathMsg;
		 else if (msg_type == message_TextMsg)
			botMsgFunction = BotClient_CS_TextMsg;
      }
   }

   (*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}
void pfnMessageEnd(void)
{
   if (gpGlobals->deathmatch)
   {
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnMessageEnd:\n"); fclose(fp); }

      if (botMsgEndFunction)
         (*botMsgEndFunction)(NULL, botMsgIndex);  // NULL indicated msg end

      // clear out the bot message function pointers...
      botMsgFunction = NULL;
      botMsgEndFunction = NULL;
   }

   (*g_engfuncs.pfnMessageEnd)();
}
void pfnWriteByte(int iValue)
{
   if (gpGlobals->deathmatch)
   {
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteByte: %d\n",iValue); fclose(fp); }

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
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteChar: %d\n",iValue); fclose(fp); }

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
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteShort: %d\n",iValue); fclose(fp); }

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
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteLong: %d\n",iValue); fclose(fp); }

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
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteAngle: %f\n",flValue); fclose(fp); }

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
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteCoord: %f\n",flValue); fclose(fp); }

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
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteString: %s\n",sz); fclose(fp); }

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
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnWriteEntity: %d\n",iValue); fclose(fp); }

      // if this message is for a bot, call the client message function...
      if (botMsgFunction)
         (*botMsgFunction)((void *)&iValue, botMsgIndex);
   }

   (*g_engfuncs.pfnWriteEntity)(iValue);
}
void pfnCVarRegister(cvar_t *pCvar)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCVarRegister:\n"); fclose(fp); }
   (*g_engfuncs.pfnCVarRegister)(pCvar);
}
float pfnCVarGetFloat(const char *szVarName)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCVarGetFloat: %s\n",szVarName); fclose(fp); }
   return (*g_engfuncs.pfnCVarGetFloat)(szVarName);
}
const char* pfnCVarGetString(const char *szVarName)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCVarGetString:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCVarGetString)(szVarName);
}
void pfnCVarSetFloat(const char *szVarName, float flValue)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCVarSetFloat:\n"); fclose(fp); }
   (*g_engfuncs.pfnCVarSetFloat)(szVarName, flValue);
}
void pfnCVarSetString(const char *szVarName, const char *szValue)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCVarSetString:\n"); fclose(fp); }
   (*g_engfuncs.pfnCVarSetString)(szVarName, szValue);
}
void* pfnPvAllocEntPrivateData(edict_t *pEdict, long cb)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPvAllocEntPrivateData:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPvAllocEntPrivateData)(pEdict, cb);
}
void* pfnPvEntPrivateData(edict_t *pEdict)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPvEntPrivateData:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPvEntPrivateData)(pEdict);
}
void pfnFreeEntPrivateData(edict_t *pEdict)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFreeEntPrivateData:\n"); fclose(fp); }
   (*g_engfuncs.pfnFreeEntPrivateData)(pEdict);
}
const char* pfnSzFromIndex(int iString)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSzFromIndex:\n"); fclose(fp); }
   return (*g_engfuncs.pfnSzFromIndex)(iString);
}
int pfnAllocString(const char *szValue)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnAllocString:\n"); fclose(fp); }
   return (*g_engfuncs.pfnAllocString)(szValue);
}
edict_t* pfnPEntityOfEntOffset(int iEntOffset)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPEntityOfEntOffset:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPEntityOfEntOffset)(iEntOffset);
}
int pfnEntOffsetOfPEntity(const edict_t *pEdict)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnEntOffsetOfPEntity: %x\n",pEdict); fclose(fp); }
   return (*g_engfuncs.pfnEntOffsetOfPEntity)(pEdict);
}
int pfnIndexOfEdict(const edict_t *pEdict)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnIndexOfEdict: %x\n",pEdict); fclose(fp); }
   return (*g_engfuncs.pfnIndexOfEdict)(pEdict);
}
edict_t* pfnPEntityOfEntIndex(int iEntIndex)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPEntityOfEntIndex:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPEntityOfEntIndex)(iEntIndex);
}
edict_t* pfnFindEntityByVars(entvars_t* pvars)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFindEntityByVars:\n"); fclose(fp); }
   return (*g_engfuncs.pfnFindEntityByVars)(pvars);
}
void* pfnGetModelPtr(edict_t* pEdict)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetModelPtr: %x\n",pEdict); fclose(fp); }
   return (*g_engfuncs.pfnGetModelPtr)(pEdict);
}
int pfnRegUserMsg(const char *pszName, int iSize)
{
   int msg;

   msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

   if (gpGlobals->deathmatch)
   {
#ifdef _DEBUG
      fp=fopen("bot.txt","a"); fprintf(fp,"pfnRegUserMsg: pszName=%s msg=%d\n",pszName,msg); fclose(fp);
#endif

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
         else if (strcmp(pszName, "Damage") == 0)
            message_Damage = msg;
         else if (strcmp(pszName, "Money") == 0)
            message_Money = msg;
         else if (strcmp(pszName, "DeathMsg") == 0)
            message_DeathMsg = msg;
         else if (strcmp(pszName, "ScreenFade") == 0)
            message_ScreenFade = msg;
		 else if (strcmp(pszName, "TextMsg") == 0)
            message_TextMsg = msg;
   }

   return msg;
}
void pfnAnimationAutomove(const edict_t* pEdict, float flTime)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnAnimationAutomove:\n"); fclose(fp); }
   (*g_engfuncs.pfnAnimationAutomove)(pEdict, flTime);
}
void pfnGetBonePosition(const edict_t* pEdict, int iBone, float *rgflOrigin, float *rgflAngles )
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetBonePosition:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetBonePosition)(pEdict, iBone, rgflOrigin, rgflAngles);
}
unsigned long pfnFunctionFromName( const char *pName )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFunctionFromName:\n"); fclose(fp); }
   return (*g_engfuncs.pfnFunctionFromName)(pName);
}
const char *pfnNameForFunction( unsigned long function )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnNameForFunction:\n"); fclose(fp); }
   return (*g_engfuncs.pfnNameForFunction)(function);
}
void pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnClientPrintf:\n"); fclose(fp); }
   (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
}
void pfnServerPrint( const char *szMsg )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnServerPrint: %s\n",szMsg); fclose(fp); }
   (*g_engfuncs.pfnServerPrint)(szMsg);
}
void pfnGetAttachment(const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles )
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetAttachment:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetAttachment)(pEdict, iAttachment, rgflOrigin, rgflAngles);
}
void pfnCRC32_Init(CRC32_t *pulCRC)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCRC32_Init:\n"); fclose(fp); }
   (*g_engfuncs.pfnCRC32_Init)(pulCRC);
}
void pfnCRC32_ProcessBuffer(CRC32_t *pulCRC, void *p, int len)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCRC32_ProcessBuffer:\n"); fclose(fp); }
   (*g_engfuncs.pfnCRC32_ProcessBuffer)(pulCRC, p, len);
}
void pfnCRC32_ProcessByte(CRC32_t *pulCRC, unsigned char ch)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCRC32_ProcessByte:\n"); fclose(fp); }
   (*g_engfuncs.pfnCRC32_ProcessByte)(pulCRC, ch);
}
CRC32_t pfnCRC32_Final(CRC32_t pulCRC)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCRC32_Final:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCRC32_Final)(pulCRC);
}
long pfnRandomLong(long lLow, long lHigh)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnRandomLong: lLow=%d lHigh=%d\n",lLow,lHigh); fclose(fp); }
   return (*g_engfuncs.pfnRandomLong)(lLow, lHigh);
}
float pfnRandomFloat(float flLow, float flHigh)
{
//   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnRandomFloat:\n"); fclose(fp); }
   return (*g_engfuncs.pfnRandomFloat)(flLow, flHigh);
}
void pfnSetView(const edict_t *pClient, const edict_t *pViewent )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSetView:\n"); fclose(fp); }
   (*g_engfuncs.pfnSetView)(pClient, pViewent);
}
float pfnTime( void )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnTime:\n"); fclose(fp); }
   return (*g_engfuncs.pfnTime)();
}
void pfnCrosshairAngle(const edict_t *pClient, float pitch, float yaw)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCrosshairAngle:\n"); fclose(fp); }
   (*g_engfuncs.pfnCrosshairAngle)(pClient, pitch, yaw);
}
byte *pfnLoadFileForMe(char *filename, int *pLength)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnLoadFileForMe: filename=%s\n",filename); fclose(fp); }
   return (*g_engfuncs.pfnLoadFileForMe)(filename, pLength);
}
void pfnFreeFile(void *buffer)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFreeFile:\n"); fclose(fp); }
   (*g_engfuncs.pfnFreeFile)(buffer);
}
void pfnEndSection(const char *pszSectionName)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnEndSection:\n"); fclose(fp); }
   (*g_engfuncs.pfnEndSection)(pszSectionName);
}
int pfnCompareFileTime(char *filename1, char *filename2, int *iCompare)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCompareFileTime:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCompareFileTime)(filename1, filename2, iCompare);
}
void pfnGetGameDir(char *szGetGameDir)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetGameDir:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetGameDir)(szGetGameDir);
}
void pfnCvar_RegisterVariable(cvar_t *variable)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCvar_RegisterVariable:\n"); fclose(fp); }
   (*g_engfuncs.pfnCvar_RegisterVariable)(variable);
}
void pfnFadeClientVolume(const edict_t *pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnFadeClientVolume:\n"); fclose(fp); }
   (*g_engfuncs.pfnFadeClientVolume)(pEdict, fadePercent, fadeOutSeconds, holdTime, fadeInSeconds);
}
/*
void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSetClientMaxspeed: edict=%x %f\n",pEdict,fNewMaxspeed); fclose(fp); }
   (*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
}
*/
void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed) {
	int index = UTIL_GetBotIndex((edict_t *)pEdict);
	if (index != -1)
		bots[index].max_speed = fNewMaxspeed;
	(*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
}
edict_t * pfnCreateFakeClient(const char *netname)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnCreateFakeClient:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCreateFakeClient)(netname);
}
void pfnRunPlayerMove(edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnRunPlayerMove:\n"); fclose(fp); }
   (*g_engfuncs.pfnRunPlayerMove)(fakeclient, viewangles, forwardmove, sidemove, upmove, buttons, impulse, msec);
}
int pfnNumberOfEntities(void)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnNumberOfEntities:\n"); fclose(fp); }
   return (*g_engfuncs.pfnNumberOfEntities)();
}
char* pfnGetInfoKeyBuffer(edict_t *e)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetInfoKeyBuffer:\n"); fclose(fp); }
   return (*g_engfuncs.pfnGetInfoKeyBuffer)(e);
}
char* pfnInfoKeyValue(char *infobuffer, char *key)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnInfoKeyValue: %s %s\n",infobuffer,key); fclose(fp); }
   return (*g_engfuncs.pfnInfoKeyValue)(infobuffer, key);
}
void pfnSetKeyValue(char *infobuffer, char *key, char *value)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSetKeyValue: %s %s\n",key,value); fclose(fp); }
   (*g_engfuncs.pfnSetKeyValue)(infobuffer, key, value);
}
void pfnSetClientKeyValue(int clientIndex, char *infobuffer, char *key, char *value)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnSetClientKeyValue: %s %s\n",key,value); fclose(fp); }
   (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
}
int pfnIsMapValid(char *filename)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnIsMapValid:\n"); fclose(fp); }
   return (*g_engfuncs.pfnIsMapValid)(filename);
}
void pfnStaticDecal( const float *origin, int decalIndex, int entityIndex, int modelIndex )
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnStaticDecal:\n"); fclose(fp); }
   (*g_engfuncs.pfnStaticDecal)(origin, decalIndex, entityIndex, modelIndex);
}
int pfnPrecacheGeneric(char* s)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnPrecacheGeneric: %s\n",s); fclose(fp); }
   return (*g_engfuncs.pfnPrecacheGeneric)(s);
}
int pfnGetPlayerUserId(edict_t *e )
{
   if (gpGlobals->deathmatch)
   {
      if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnGetPlayerUserId: %x\n",e); fclose(fp); }
   }

   return (*g_engfuncs.pfnGetPlayerUserId)(e);
}
void pfnBuildSoundMsg(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnBuildSoundMsg:\n"); fclose(fp); }
   (*g_engfuncs.pfnBuildSoundMsg)(entity, channel, sample, volume, attenuation, fFlags, pitch, msg_dest, msg_type, pOrigin, ed);
}
int pfnIsDedicatedServer(void)
{
   if (debug_engine) { fp=fopen("bot.txt","a"); fprintf(fp,"pfnIsDedicatedServer:\n"); fclose(fp); }
   return (*g_engfuncs.pfnIsDedicatedServer)();
}
