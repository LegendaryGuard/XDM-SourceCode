#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "soundent.h"
#include "decals.h"
#include "shake.h"

//DLL_GLOBAL ULONG		g_ulFrameCount;
DLL_GLOBAL ULONG		g_ulModelIndexPlayer = 1;
DLL_GLOBAL Vector		g_vecAttackDir;// XDM3038a: Attack direction vector. This old hack is now used to transmit precise direction from RadiusDamage() to TakeDamage() for pushing. MUST BE NORMALIZED!
//DLL_GLOBAL int			g_Language = 0;

DLL_GLOBAL unsigned short g_usBulletImpact;
DLL_GLOBAL unsigned short g_usBeamImpact;
DLL_GLOBAL unsigned short g_usExplosion;
DLL_GLOBAL unsigned short g_usFlame;
DLL_GLOBAL unsigned short g_usFlameTrail;
DLL_GLOBAL unsigned short g_usGrenExp;
DLL_GLOBAL unsigned short g_usSparkShower;
DLL_GLOBAL unsigned short g_usTrail;
DLL_GLOBAL unsigned short g_usWarpBall;
		   
DLL_GLOBAL unsigned short g_usCaptureObject;
DLL_GLOBAL unsigned short g_usDomPoint;
DLL_GLOBAL unsigned short g_usItemSpawn;
DLL_GLOBAL unsigned short g_usPlayerSpawn;
DLL_GLOBAL unsigned short g_usTeleport;
DLL_GLOBAL unsigned short g_usNuclearDevice;
DLL_GLOBAL unsigned short g_usMonFx;

//extern "C"// needed only when defined in .C files
//{
DLL_GLOBAL unsigned short g_usPM_Fall;
DLL_GLOBAL unsigned short g_usPM_Longjump;
//}

int gmsgInitHUD = 0;
int gmsgResetHUD = 0;
//int gmsgCurWeapon = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgFlashlight = 0;
//int gmsgFlashBattery = 0;
int gmsgShowGameTitle = 0;
//XDM3037aint gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgDamageFx = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoList = 0;// XDM3037
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgTeamNames = 0;
int gmsgDomInfo = 0;// XDM
int gmsgFlagInfo = 0;
int gmsgGameMode = 0;
int gmsgGRInfo = 0;
int gmsgGREvent = 0;// XDM3035: awards, combobreakers )
int gmsgMOTD = 0;
int gmsgServerName = 0;
//XDM3038a int gmsgAmmoPickup = 0;
//XDM3038a int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
//XDM3037a int gmsgSetFOV = 0;
int gmsgViewMode = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgStatusData = 0;// XDM3037
int gmsgStatusIcon = 0;
int gmsgViewModel = 0;// XDM
int gmsgParticles = 0;
int gmsgPartSys = 0;
int gmsgRenderSys = 0;// XDM3038b
int gmsgSpectator = 0;
int gmsgSetFog = 0;// XDM
int gmsgSetSky = 0;// XDM
int gmsgSetRain = 0;// XDM
int gmsgAudioTrack = 0;
int gmsgFireBeam = 0;// XDM
int gmsgStaticEnt = 0;// XDM3034
int gmsgStaticSprite = 0;// XDM3035a
int gmsgSpawnGibs = 0;
int gmsgUpdWeapons = 0;// XDM3035
int gmsgUpdAmmo = 0;
int gmsgTEModel = 0;// XDM3035a
int gmsgBubbles = 0;
int gmsgSpeakSnd = 0;
int gmsgPickedEnt = 0;
int gmsgItemSpawn = 0;// XDM3035c
int gmsgSelBestItem = 0;
int gmsgPlayerStats = 0;// XDM3037
int gmsgTeleport = 0;// XDM3037
int gmsgHUDEffects = 0;// XDM3038
int gmsgHUDTimer = 0;// XDM3038c


//-----------------------------------------------------------------------------
// Purpose: Register server-to-client (S2C) network messages
// Warning: Call this from Precache() to avoid error messages!
//-----------------------------------------------------------------------------
void LinkUserMessages(void)
{
	// Already taken care of?
	if (gmsgInitHUD)
		return;

	// WARNING! Message name must not be longer than 11 characters!
	gmsgInitHUD			= REG_USER_MSG("InitHUD", 0);// called every time a new player joins the server
	gmsgResetHUD		= REG_USER_MSG("ResetHUD", 0);// called every respawn // XDM3035c
	//gmsgCurWeapon		= REG_USER_MSG("CurWeapon", 3);
	gmsgGeigerRange		= REG_USER_MSG("Geiger", 1);
	gmsgFlashlight		= REG_USER_MSG("Flashlight", 2);
	//economy	gmsgFlashBattery	= REG_USER_MSG("FlashBat", 1);
	//XDM3037a	gmsgHealth			= REG_USER_MSG("Health", 1);
	gmsgDamage			= REG_USER_MSG("Damage", 11);// XDM3038c
	gmsgDamageFx		= REG_USER_MSG("DamageFx", 8);// short + long
	gmsgBattery			= REG_USER_MSG("Battery", 1);// XDM3038: was 2 for SHORT
	gmsgTrain			= REG_USER_MSG("Train", 1);
	gmsgSayText			= REG_USER_MSG("SayText", -1);
	gmsgTextMsg			= REG_USER_MSG("TextMsg", -1);
	gmsgWeaponList		= REG_USER_MSG("WeaponList", -1);// XDM3037 UPDATE!
	gmsgAmmoList		= REG_USER_MSG("AmmoList", -1);// XDM3037
	gmsgShowGameTitle	= REG_USER_MSG("GameTitle", 1);// XDM3038c: -1?
	gmsgDeathMsg		= REG_USER_MSG("DeathMsg", -1);
	gmsgScoreInfo		= REG_USER_MSG("ScoreInfo", 5);// XDM: length changed from 9 // In case someone needs score more than +-32768, change this
	gmsgTeamInfo		= REG_USER_MSG("TeamInfo", 2);// XDM: length changed from -1. associates player with team
	gmsgTeamScore		= REG_USER_MSG("TeamScore", 3);// XDM: length changed from -1. sets the score of a team on the scoreboard
	gmsgTeamNames		= REG_USER_MSG("TeamNames", -1);
	gmsgDomInfo			= REG_USER_MSG("DomInfo", -1);
	gmsgFlagInfo		= REG_USER_MSG("FlagInfo", 3);
	gmsgGameMode		= REG_USER_MSG("GameMode", -1);// UPDATE: XDM3035c had 9. Extend as required.
	gmsgGRInfo			= REG_USER_MSG("GRInfo", 8);// XDM3038
	gmsgGREvent			= REG_USER_MSG("GREvent", 5);// XDM3035 1+2+2
	gmsgMOTD			= REG_USER_MSG("MOTD", -1);
	gmsgServerName		= REG_USER_MSG("ServerName", -1);
	//XDM3038a 	gmsgAmmoPickup		= REG_USER_MSG("AmmoPickup", 2);
	//gmsgWeapPickup		= REG_USER_MSG("WeapPickup", 1);
	gmsgItemPickup		= REG_USER_MSG("ItemPickup", -1);
	gmsgHideWeapon		= REG_USER_MSG("HideWeapon", 1);
	//XDM3037a	gmsgSetFOV			= REG_USER_MSG("SetFOV", 1);
	//economy	gmsgViewMode		= REG_USER_MSG("ViewMode", 1);
	gmsgShowMenu		= REG_USER_MSG("ShowMenu", 2);
	gmsgShake			= REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade			= REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgStatusData		= REG_USER_MSG("StatusData", -1);// status bar
	gmsgStatusIcon		= REG_USER_MSG("StatusIcon", -1);
	gmsgViewModel		= REG_USER_MSG("ViewModel", 7);// XDM
	gmsgParticles		= REG_USER_MSG("Particles", 12);// XDM: old style engine particles
	gmsgPartSys			= REG_USER_MSG("PartSys", 30);// XDM3030-style particle system message
	gmsgRenderSys		= REG_USER_MSG("RenderSys", -1);// XDM3038b: scripted RenderSystem
	gmsgSpectator		= REG_USER_MSG("Spectator", 2);// byte player, byte specmode
	gmsgSetFog			= REG_USER_MSG("SetFog", 8);
	gmsgSetSky			= REG_USER_MSG("SetSky", 7);
	gmsgSetRain			= REG_USER_MSG("SetRain", 29);// XDM3034 + 2 + 4 bytes // XDM3035c +2
	gmsgAudioTrack		= REG_USER_MSG("AudioTrack", -1);// XDM3038: sizeless
	gmsgFireBeam		= REG_USER_MSG("FireBeam", 17);
	gmsgStaticEnt		= REG_USER_MSG("StaticEnt", -1);// XDM3038b 33);// XDM3035
	gmsgStaticSprite	= REG_USER_MSG("StaticSpr", 24);// XDM3037a: update: removed renderfx
	//gmsgSpawnGibs		= REG_USER_MSG("SpawnGibs", 32);// UNDONE
	gmsgUpdWeapons		= REG_USER_MSG("UpdWeapons", -1);
	gmsgUpdAmmo			= REG_USER_MSG("UpdAmmo", -1);
	gmsgTEModel			= REG_USER_MSG("TEModel", 18);// XDM3035a
	gmsgBubbles			= REG_USER_MSG("Bubbles", 14);// XDM3035c
	gmsgSpeakSnd		= REG_USER_MSG("SpeakSnd", -1);// XDM3035a
	gmsgPickedEnt		= REG_USER_MSG("PickedEnt", 8);// XDM3035a
	gmsgItemSpawn		= REG_USER_MSG("ItemSpawn", 18);// XDM3035c: events do not always play
#if !defined(SERVER_WEAPON_SLOTS)
	gmsgSelBestItem		= REG_USER_MSG("SelBestItem", 1);// XDM3035c
#endif
	gmsgPlayerStats		= REG_USER_MSG("PlayerStats", -1);// XDM3037
	gmsgTeleport		= REG_USER_MSG("Teleport", 9);// XDM3037
	gmsgHUDEffects		= REG_USER_MSG("HUDEffects", 2);// XDM3038
	gmsgHUDTimer		= REG_USER_MSG("HUDTimer", -1);// XDM3038c: time + title string
	// WARNING! Protocol 46 supports maximum of 128 messages! Don't forget CVoiceGameMgr::Init() adds some too. "Host_Error: DispatchUserMsg:  Illegal User Msg 128"
}

//-----------------------------------------------------------------------------
// Purpose: Register global network "events"
//-----------------------------------------------------------------------------
void PrecacheEvents(void)
{
	g_usBulletImpact	= PRECACHE_EVENT(1, "events/fx/blimpact.sc");
	g_usBeamImpact		= PRECACHE_EVENT(1, "events/fx/bmimpact.sc");
	g_usExplosion		= PRECACHE_EVENT(1, "events/fx/explosion.sc");
	g_usFlame			= PRECACHE_EVENT(1, "events/fx/flame.sc");
	g_usFlameTrail		= PRECACHE_EVENT(1, "events/fx/flametrail.sc");
	g_usGrenExp			= PRECACHE_EVENT(1, "events/fx/gren_exp.sc");
	g_usSparkShower		= PRECACHE_EVENT(1, "events/fx/sparkshower.sc");
	g_usTrail			= PRECACHE_EVENT(1, "events/fx/trail.sc");
	g_usWarpBall		= PRECACHE_EVENT(1, "events/fx/warpball.sc");

	g_usCaptureObject	= PRECACHE_EVENT(1, "events/game/captureobject.sc");
	g_usDomPoint		= PRECACHE_EVENT(1, "events/game/dompoint.sc");
	//g_usItemSpawn		= PRECACHE_EVENT(1, "events/game/itemspawn.sc");
	g_usPlayerSpawn		= PRECACHE_EVENT(1, "events/game/playerspawn.sc");
	//g_usTeleport		= PRECACHE_EVENT(1, "events/game/teleport.sc");

	g_usNuclearDevice	= PRECACHE_EVENT(1, "events/weapons/nucleardev.sc");// XDM3035

	g_usPM_Fall			= PRECACHE_EVENT(1, "events/pm/fall.sc");// XDM3035a
	g_usPM_Longjump		= PRECACHE_EVENT(1, "events/pm/longjump.sc");// XDM3035b
}
