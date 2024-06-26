//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>// XDM: isdigit
#include "parsemsg.h"
#include "hud_servers.h"
#include "demo.h"
#include "demo_api.h"
#include "voice_status.h"
#include "r_efx.h"
#include "entity_types.h"
#include "VGUI_ActionSignal.h"
#include "VGUI_Scheme.h"
#include "VGUI_TextImage.h"
#include "vgui_loadtga.h"
//#include "vgui_helpers.h"
#include "VGUI_MouseCode.h"
#include "Exports.h"
#include "RenderManager.h"



using namespace vgui;


extern int cam_thirdperson;


#define VOICE_MODEL_INTERVAL		0.3
#define SCOREBOARD_BLINK_FREQUENCY	0.3	// How often to blink the scoreboard icons.
#define SQUELCHOSCILLATE_PER_SECOND	2.0f


//extern BitmapTGA *LoadTGA( const char* pImageName );



// ---------------------------------------------------------------------- //
// The voice manager for the client.
// ---------------------------------------------------------------------- //
CVoiceStatus g_VoiceStatus;

CVoiceStatus* GetClientVoiceMgr()
{
	return &g_VoiceStatus;
}



// ---------------------------------------------------------------------- //
// CVoiceStatus.
// ---------------------------------------------------------------------- //

static CVoiceStatus *g_pInternalVoiceStatus = NULL;

int __MsgFunc_VoiceMask(const char *pszName, int iSize, void *pbuf)
{
	if (g_pInternalVoiceStatus)
		g_pInternalVoiceStatus->HandleVoiceMaskMsg(iSize, pbuf);

	return 1;
}

int __MsgFunc_ReqState(const char *pszName, int iSize, void *pbuf)
{
	if (g_pInternalVoiceStatus)
		g_pInternalVoiceStatus->HandleReqStateMsg(iSize, pbuf);

	return 1;
}


int g_BannedPlayerPrintCount;
void PrintBannedPlayer(char id[16])
{
	char str[256];
	_snprintf(str, 256, "Ban %d: %2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x%2x\n\0",
		g_BannedPlayerPrintCount++,
		id[0], id[1], id[2], id[3], 
		id[4], id[5], id[6], id[7], 
		id[8], id[9], id[10], id[11], 
		id[12], id[13], id[14], id[15]
		);
	str[255] = '\0';
#ifdef _WIN32
	_strupr(str);
#endif
	gEngfuncs.pfnConsolePrint(str);
}


void ShowBannedCallback()
{
	if(g_pInternalVoiceStatus)
	{
		g_BannedPlayerPrintCount = 0;
		gEngfuncs.pfnConsolePrint("------- BANNED PLAYERS -------\n");
		g_pInternalVoiceStatus->m_BanMgr.ForEachBannedPlayer(PrintBannedPlayer);
		gEngfuncs.pfnConsolePrint("------------------------------\n");
	}
}


// ---------------------------------------------------------------------- //
// CVoiceStatus.
// ---------------------------------------------------------------------- //
CVoiceStatus::CVoiceStatus()
{
	m_bBanMgrInitialized = false;
	m_LastUpdateServerState = 0;

	m_pSpeakerLabelIcon = NULL;
	m_pScoreboardNeverSpoken = NULL;
	m_pScoreboardNotSpeaking = NULL;
	m_pScoreboardSpeaking = NULL;
	m_pScoreboardSpeaking2 = NULL;
	m_pScoreboardSquelch = NULL;
	m_pScoreboardBanned = NULL;
	
	m_pLocalBitmap = NULL;
	m_pAckBitmap = NULL;

	m_bTalking = m_bServerAcked = false;

	memset(m_pBanButtons, 0, sizeof(m_pBanButtons));

	m_pParentPanel = NULL;

	m_bServerModEnable = -1;

	//m_pchGameDir = NULL;
	m_pchGameDir[0] = 0;// XDM3038b
	m_pCvarModEnable = NULL;// XDM
	m_pCvarClientDebug = NULL;

	/*for (size_t i=0; i < MAX_VOICE_SPEAKERS; i++)// XDM3038a: ?
	{
		m_pVoiceHeadModels[i] = new CRSSprite(...);
		g_pRenderManager->AddSystem(m_pVoiceHeadModels[i]);
	}*/
}


CVoiceStatus::~CVoiceStatus()
{
	g_pInternalVoiceStatus = NULL;
	
	for(short i=0; i < MAX_VOICE_SPEAKERS; i++)
	{
		delete m_Labels[i].m_pLabel;
		m_Labels[i].m_pLabel = NULL;

		delete m_Labels[i].m_pIcon;
		m_Labels[i].m_pIcon = NULL;
		
		delete m_Labels[i].m_pBackground;
		m_Labels[i].m_pBackground = NULL;
	}				

	if (m_pLocalLabel)
	{
		delete m_pLocalLabel;
		m_pLocalLabel = NULL;
	}

	FreeBitmaps();

	if (m_pchGameDir && *m_pchGameDir)
	{
		if(m_bBanMgrInitialized)
		{
			m_BanMgr.SaveState(m_pchGameDir);
		}
		//XDM3038b: OBSOELTE	free(m_pchGameDir);
	}
}


int CVoiceStatus::Init(
	IVoiceStatusHelper *pHelper,
	Panel **pParentPanel)
{
	//DBG_PRINTF("CVoiceStatus::Init()\n");
	conprintf(2, "CVoiceStatus::Init()\n");
	// Setup the voice_modenable cvar.
	m_pCvarModEnable = CVAR_CREATE("voice_modenable", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL);// XDM
	m_pCvarClientDebug = CVAR_CREATE("voice_clientdebug", "0", FCVAR_CLIENTDLL);

	gEngfuncs.pfnAddCommand("voice_showbanned", ShowBannedCallback);

	if (gEngfuncs.pfnGetGameDirectory())
	{
		m_BanMgr.Init(gEngfuncs.pfnGetGameDirectory());
		m_bBanMgrInitialized = true;
	}

	assert(!g_pInternalVoiceStatus);
	g_pInternalVoiceStatus = this;

	m_BlinkTimer = 0;
//	m_VoiceHeadModel = NULL;
	memset(m_Labels, 0, sizeof(m_Labels));
	
	for(short i=0; i < MAX_VOICE_SPEAKERS; ++i)
	{
		CVoiceLabel *pLabel = &m_Labels[i];
		try
		{
			pLabel->m_pBackground = new Label("");
		}
		catch (...)
		{
			//pLabel->m_pBackground = NULL;
			break;
		}
		try
		{
			pLabel->m_pLabel = new Label("");
		}
		catch (...)
		{
			//pLabel->m_pLabel = NULL;
			break;
		}
		if (pLabel->m_pLabel)
		{
			pLabel->m_pLabel->setVisible( true );
			pLabel->m_pLabel->setFont( Scheme::sf_primary2 );
			pLabel->m_pLabel->setTextAlignment( Label::a_east );
			pLabel->m_pLabel->setContentAlignment( Label::a_east );
			pLabel->m_pLabel->setParent( pLabel->m_pBackground );
		}
		pLabel->m_pIcon = new ImagePanel(NULL);
		if (pLabel->m_pIcon)
		{
			pLabel->m_pIcon->setVisible( true );
			pLabel->m_pIcon->setParent( pLabel->m_pBackground );
		}
		pLabel->m_clientindex = -1;
	}

	m_pLocalLabel = new ImagePanel(NULL);

	m_bInSquelchMode = false;

	m_pHelper = pHelper;
	m_pParentPanel = pParentPanel;
	//gHUD.AddHudElem(this);
	//m_iFlags = HUD_ACTIVE;
	HOOK_MESSAGE(VoiceMask);
	HOOK_MESSAGE(ReqState);

	// Cache the game directory for use when we shut down
	//const char *pchGameDirT = gEngfuncs.pfnGetGameDirectory();
	// OBSOELTE m_pchGameDir = (char *)malloc(strlen(pchGameDirT) + 1);
	strncpy(m_pchGameDir, gEngfuncs.pfnGetGameDirectory(), MAX_PATH);// XDM3038b: hate dynamic allocations in such small places!

	return 1;
}


int CVoiceStatus::VidInit()
{
	conprintf(2, "CVoiceStatus::VidInit()\n");
	FreeBitmaps();

	m_pLocalBitmap = vgui_LoadTGA("gfx/vgui/icntlk_pl.tga");
	if (m_pLocalBitmap)
		m_pLocalBitmap->setColor(vgui::Color(255,255,255,135));

	m_pAckBitmap = vgui_LoadTGA("gfx/vgui/icntlk_sv.tga");
	if (m_pAckBitmap)
		m_pAckBitmap->setColor(vgui::Color(255,255,255,135));	// Give just a tiny bit of translucency so software draws correctly.

	m_pLocalLabel->setImage( m_pLocalBitmap );
	m_pLocalLabel->setVisible( false );

	m_pSpeakerLabelIcon = vgui_LoadTGANoInvertAlpha("gfx/vgui/speaker4.tga");
	if (m_pSpeakerLabelIcon)
		m_pSpeakerLabelIcon->setColor(vgui::Color(255,255,255,1));		// Give just a tiny bit of translucency so software draws correctly.

	m_pScoreboardNeverSpoken = vgui_LoadTGANoInvertAlpha("gfx/vgui/640_speaker1.tga");
	if (m_pScoreboardNeverSpoken)
		m_pScoreboardNeverSpoken->setColor(vgui::Color(255,255,255,1));	// Give just a tiny bit of translucency so software draws correctly.

	m_pScoreboardNotSpeaking = vgui_LoadTGANoInvertAlpha("gfx/vgui/640_speaker2.tga");
	if (m_pScoreboardNotSpeaking)
		m_pScoreboardNotSpeaking->setColor(vgui::Color(255,255,255,1));	// Give just a tiny bit of translucency so software draws correctly.

	m_pScoreboardSpeaking = vgui_LoadTGANoInvertAlpha("gfx/vgui/640_speaker3.tga");
	if (m_pScoreboardSpeaking)
		m_pScoreboardSpeaking->setColor(vgui::Color(255,255,255,1));	// Give just a tiny bit of translucency so software draws correctly.

	m_pScoreboardSpeaking2 = vgui_LoadTGANoInvertAlpha("gfx/vgui/640_speaker4.tga");
	if (m_pScoreboardSpeaking2)
		m_pScoreboardSpeaking2->setColor(vgui::Color(255,255,255,1));	// Give just a tiny bit of translucency so software draws correctly.

	m_pScoreboardSquelch  = vgui_LoadTGA("gfx/vgui/icntlk_squelch.tga");
	if (m_pScoreboardSquelch)
		m_pScoreboardSquelch->setColor(vgui::Color(255,255,255,1));	// Give just a tiny bit of translucency so software draws correctly.

	m_pScoreboardBanned = vgui_LoadTGA("gfx/vgui/640_voiceblocked.tga");
	if (m_pScoreboardBanned)
		m_pScoreboardBanned->setColor(vgui::Color(255,255,255,1));	// Give just a tiny bit of translucency so software draws correctly.

	// XDM: voice icon is now drawn properly
	return TRUE;
}


void CVoiceStatus::Frame(double frametime)
{
	// check server banned players once per second
	if(gEngfuncs.GetClientTime() - m_LastUpdateServerState > 1)
	{
		UpdateServerState(false);
	}

	m_BlinkTimer += frametime;

	// Update speaker labels.
	if (m_pHelper && m_pHelper->CanShowSpeakerLabels())// XDM3038a
	{
		for( int i=0; i < MAX_VOICE_SPEAKERS; i++ )
			m_Labels[i].m_pBackground->setVisible( m_Labels[i].m_clientindex != -1 );
	}
	else
	{
		for( int i=0; i < MAX_VOICE_SPEAKERS; i++ )
			m_Labels[i].m_pBackground->setVisible( false );
	}

	for(int i=0; i < VOICE_MAX_PLAYERS; i++)
		UpdateBanButton(i);
}

void CVoiceStatus::UpdateSpeakerStatus( int entindex, qboolean bTalking )
{
	if ( !m_pParentPanel || !*m_pParentPanel )
	{
		return;
	}

	if (m_pCvarClientDebug->value > 0.0f)
	{
		char msg[256];
		_snprintf(msg, 256, "CVoiceStatus::UpdateSpeakerStatus: ent %d talking = %d\n", entindex, bTalking);
		gEngfuncs.pfnConsolePrint( msg );
	}

	int iLocalPlayerIndex = gEngfuncs.GetLocalPlayer()->index;
	cvar_t *pVoiceLoopback = NULL;

	// Is it the local player talking?
	if ( entindex == -1 )
	{
		m_bTalking = !!bTalking;
		if( bTalking )
		{
			// Enable voice for them automatically if they try to talk.
			gEngfuncs.pfnClientCmd( "voice_modenable 1" );
		}

		// now set the player index to the correct index for the local player
		// this will allow us to have the local player's icon flash in the scoreboard
		entindex = iLocalPlayerIndex;

		pVoiceLoopback = gEngfuncs.pfnGetCvarPointer( "voice_loopback" );
	}
	else if ( entindex == -2 )
	{
		m_bServerAcked = !!bTalking;
	}

	if ( entindex >= 0 && entindex <= VOICE_MAX_PLAYERS )
	{
		int iClient = entindex - 1;
		if ( iClient < 0 )
		{
			return;
		}

		CVoiceLabel *pLabel = FindVoiceLabel( iClient );
		if ( bTalking )
		{
			m_VoicePlayers[iClient] = true;
			m_VoiceEnabledPlayers[iClient] = true;

			// If we don't have a label for this guy yet, then create one.
			if ( !pLabel )
			{
				// if this isn't the local player (unless they have voice_loopback on)
				if ( ( entindex != iLocalPlayerIndex ) || ( pVoiceLoopback && pVoiceLoopback->value ) )
				{
					if ((pLabel = GetFreeVoiceLabel()) != NULL)
					{
						// Get the name from the engine.
						hud_player_info_t info;
						memset( &info, 0, sizeof( info ) );
						gEngfuncs.pfnGetPlayerInfo( entindex, &info );

						char paddedName[512];
						_snprintf(paddedName, 512, "%s   ", info.name);

						int color[3];
						m_pHelper->GetPlayerTextColor( entindex, color );

						if ( pLabel->m_pBackground )
						{
							pLabel->m_pBackground->setBgColor( color[0], color[1], color[2], 135 );
							pLabel->m_pBackground->setParent( *m_pParentPanel );
							pLabel->m_pBackground->setVisible( m_pHelper->CanShowSpeakerLabels() );
						}

						if ( pLabel->m_pLabel )
						{
							pLabel->m_pLabel->setFgColor( 255, 255, 255, 0 );
							pLabel->m_pLabel->setBgColor( 0, 0, 0, 255 );
							pLabel->m_pLabel->setText( "%s", paddedName );
						}

						pLabel->m_clientindex = iClient;
					}
				}
			}
		}
		else
		{
			m_VoicePlayers[iClient] = false;

			// If we have a label for this guy, kill it.
			if ( pLabel )
			{
				pLabel->m_pBackground->setVisible( false );
				pLabel->m_clientindex = -1;
			}
		}
	}

	RepositionLabels();
}


void CVoiceStatus::UpdateServerState(bool bForce)
{
	// Can't do anything when we're not in a level.
	char const *pLevelName = gEngfuncs.pfnGetLevelName();
	if( pLevelName[0] == 0 )
	{
		if (m_pCvarClientDebug->value > 0.0f)
		{
			gEngfuncs.pfnConsolePrint( "CVoiceStatus::UpdateServerState: pLevelName[0]==0\n" );
		}
		return;
	}
	
	int bCVarModEnable = !!gEngfuncs.pfnGetCvarFloat("voice_modenable");
	if(bForce || m_bServerModEnable != bCVarModEnable)
	{
		m_bServerModEnable = bCVarModEnable;

		char str[256];
		_snprintf(str, 256, "VModEnable %d\n", m_bServerModEnable);
		SERVER_COMMAND(str);
		if (m_pCvarClientDebug->value > 0.0f)
		{
			char msg[256];
			_snprintf(msg, 256, "CVoiceStatus::UpdateServerState: Sending '%s'\n", str);
			gEngfuncs.pfnConsolePrint(msg);
		}
	}

	char str[2048];
	sprintf(str, "vban");
	bool bChange = false;

	for(unsigned long dw=0; dw < VOICE_MAX_PLAYERS_DW; ++dw)
	{	
		unsigned long serverBanMask = 0;
		unsigned long banMask = 0;
		for(unsigned long i=0; i < 32; i++)
		{
			char playerID[16];
			if(!gEngfuncs.GetPlayerUniqueID(i+1, playerID))
				continue;

			if(m_BanMgr.GetPlayerBan(playerID))
				banMask |= 1 << i;

			if(m_ServerBannedPlayers[dw*32 + i])
				serverBanMask |= 1 << i;
		}

		if(serverBanMask != banMask)
			bChange = true;

		// Ok, the server needs to be updated.
		char numStr[512];
		_snprintf(numStr, 512, " %x", banMask);
		numStr[511] = '\0';
		strncat(str, numStr, 512);
	}

	if(bChange || bForce)
	{
		if (m_pCvarClientDebug->value > 0.0f)
		{
			char msg[256];
			_snprintf(msg, 256, "CVoiceStatus::UpdateServerState: Sending '%s'\n", str);
			gEngfuncs.pfnConsolePrint(msg);
		}

		gEngfuncs.pfnServerCmdUnreliable(str);	// Tell the server..
	}
	else
	{
		if (m_pCvarClientDebug->value > 0.0f)
		{
			gEngfuncs.pfnConsolePrint( "CVoiceStatus::UpdateServerState: no change\n" );
		}
	}
	
	m_LastUpdateServerState = gEngfuncs.GetClientTime();
}

void CVoiceStatus::UpdateSpeakerImage(Label *pLabel, int iPlayer)
{
	// XDM: this WHOLE voice code needs to be revisited!
	//conprintf(1, "UpdateSpeakerImage(%d)\n", iPlayer);
	m_pBanButtons[iPlayer-1] = pLabel;
	UpdateBanButton(iPlayer-1);
}

// XDM: ?!!
bool HACK_GetPlayerUniqueID( int iPlayer, char playerID[16] )
{
	return !!gEngfuncs.GetPlayerUniqueID( iPlayer, playerID );
}

void CVoiceStatus::UpdateBanButton(int iClient)
{
//	ASSERT(iClient > 0);
	Label *pPanel = m_pBanButtons[iClient];// XDM3035c this is 0-based index

	if (!pPanel)
		return;

	char playerID[16];
	//extern bool HACK_GetPlayerUniqueID( int iPlayer, char playerID[16] );
	//if(!HACK_GetPlayerUniqueID(iClient+1, playerID))
	if(!HACK_GetPlayerUniqueID(iClient, playerID))
		return;

	// Figure out if it's blinking or not.
	bool bBlink   = fmod(m_BlinkTimer, SCOREBOARD_BLINK_FREQUENCY*2) < SCOREBOARD_BLINK_FREQUENCY;
	bool bTalking = !!m_VoicePlayers[iClient];
	bool bBanned  = m_BanMgr.GetPlayerBan(playerID);
	bool bNeverSpoken = !m_VoiceEnabledPlayers[iClient];

	// Get the appropriate image to display on the panel.
	if (bBanned)
	{
		pPanel->setImage(m_pScoreboardBanned);
		//conprintf(1, "UpdateBanButton(%d) banned\n", iClient);
	}
	else if (bTalking)
	{
		if (bBlink)
		{
			pPanel->setImage(m_pScoreboardSpeaking2);
		}
		else
		{
			pPanel->setImage(m_pScoreboardSpeaking);
		}
		pPanel->setFgColor(255, 255, 255, 1);
	}
	else if (bNeverSpoken)
	{
		pPanel->setImage(m_pScoreboardNeverSpoken);
		pPanel->setFgColor(95, 95, 95, 1);
		//conprintf(1, "UpdateBanButton(%d) never spk\n", iClient);
	}
	else
	{
		pPanel->setImage(m_pScoreboardNotSpeaking);
		//conprintf(1, "UpdateBanButton(%d) not spk\n", iClient);
	}
}


void CVoiceStatus::HandleVoiceMaskMsg(int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	unsigned long dw;
	for(dw=0; dw < VOICE_MAX_PLAYERS_DW; dw++)
	{
		m_AudiblePlayers.SetDWord(dw, (unsigned long)READ_LONG());
		m_ServerBannedPlayers.SetDWord(dw, (unsigned long)READ_LONG());

		if (m_pCvarClientDebug->value > 0.0f)
		{
			char str[256];
			gEngfuncs.pfnConsolePrint("CVoiceStatus::HandleVoiceMaskMsg\n");

			_snprintf(str, 256, "    - m_AudiblePlayers[%d] = %lu\n", dw, m_AudiblePlayers.GetDWord(dw));
			gEngfuncs.pfnConsolePrint(str);

			_snprintf(str, 256, "    - m_ServerBannedPlayers[%d] = %lu\n", dw, m_ServerBannedPlayers.GetDWord(dw));
			gEngfuncs.pfnConsolePrint(str);
		}
	}

	m_bServerModEnable = READ_BYTE();
}

void CVoiceStatus::HandleReqStateMsg(int iSize, void *pbuf)
{
	if (m_pCvarClientDebug->value > 0.0f)
	{
		gEngfuncs.pfnConsolePrint("CVoiceStatus::HandleReqStateMsg\n");
	}

	UpdateServerState(true);	
}

void CVoiceStatus::StartSquelchMode()
{
	if(m_bInSquelchMode)
		return;

	m_bInSquelchMode = true;
	m_pHelper->UpdateCursorState();
}

void CVoiceStatus::StopSquelchMode()
{
	m_bInSquelchMode = false;
	m_pHelper->UpdateCursorState();
}

bool CVoiceStatus::IsInSquelchMode()
{
	return m_bInSquelchMode;
}

CVoiceLabel* CVoiceStatus::FindVoiceLabel(int clientindex)
{
	for(int i=0; i < MAX_VOICE_SPEAKERS; ++i)
	{
		if(m_Labels[i].m_clientindex == clientindex)
			return &m_Labels[i];
	}

	return NULL;
}


CVoiceLabel* CVoiceStatus::GetFreeVoiceLabel()
{
	return FindVoiceLabel(-1);
}


void CVoiceStatus::RepositionLabels()
{
	// find starting position to draw from, along right-hand side of screen
	int y = ScreenHeight / 2;
	
	int iconWide = 8, iconTall = 8;
	if( m_pSpeakerLabelIcon )
	{
		m_pSpeakerLabelIcon->getSize( iconWide, iconTall );
	}
	
	// Reposition active labels.
	for(int i = 0; i < MAX_VOICE_SPEAKERS; i++)
	{
		CVoiceLabel *pLabel = &m_Labels[i];

		if( pLabel->m_clientindex == -1 || !pLabel->m_pLabel )
		{
			if( pLabel->m_pBackground )
				pLabel->m_pBackground->setVisible( false );

			continue;
		}

		int textWide, textTall;
		pLabel->m_pLabel->getContentSize( textWide, textTall );

		// Don't let it stretch too far across their screen.
		if( textWide > (ScreenWidth*2)/3 )
			textWide = (ScreenWidth*2)/3;

		// Setup the background label to fit everything in.
		int border = 2;
		int bgWide = textWide + iconWide + border*3;
		int bgTall = max( textTall, iconTall ) + border*2;
		pLabel->m_pBackground->setBounds( ScreenWidth - bgWide - 8, y, bgWide, bgTall );

		// Put the text at the left.
		pLabel->m_pLabel->setBounds( border, (bgTall - textTall) / 2, textWide, textTall );

		// Put the icon at the right.
		int iconLeft = border + textWide + border;
		int iconTop = (bgTall - iconTall) / 2;
		if( pLabel->m_pIcon )
		{
			pLabel->m_pIcon->setImage( m_pSpeakerLabelIcon );
			pLabel->m_pIcon->setBounds( iconLeft, iconTop, iconWide, iconTall );
		}

		y += bgTall + 2;
	}

	if( m_pLocalBitmap && m_pAckBitmap && m_pLocalLabel && (m_bTalking || m_bServerAcked) )
	{
		m_pLocalLabel->setParent(*m_pParentPanel);
		m_pLocalLabel->setVisible( true );

		if( m_bServerAcked && m_pCvarClientDebug->value > 0.0f)//!!gEngfuncs.pfnGetCvarFloat("voice_clientdebug") )
			m_pLocalLabel->setImage( m_pAckBitmap );
		else
			m_pLocalLabel->setImage( m_pLocalBitmap );

		int sizeX, sizeY;
		m_pLocalBitmap->getSize(sizeX, sizeY);

		int local_xPos = ScreenWidth - sizeX - 10;
		int local_yPos = m_pHelper->GetAckIconHeight() - sizeY;
		
		m_pLocalLabel->setPos( local_xPos, local_yPos );
	}
	else
	{
		m_pLocalLabel->setVisible( false );
	}
}


void CVoiceStatus::FreeBitmaps()
{
	// Clear references to the images in panels.
	for (int i=0; i < VOICE_MAX_PLAYERS; i++)
	{
		if (m_pBanButtons[i])
			m_pBanButtons[i]->setImage(NULL);
	}

	if (m_pLocalLabel)
		m_pLocalLabel->setImage(NULL);

	// Delete all the images we have loaded.
	if (m_pLocalBitmap)
	{
		delete m_pLocalBitmap;
		m_pLocalBitmap = NULL;
	}
	if (m_pAckBitmap)
	{
		delete m_pAckBitmap;
		m_pAckBitmap = NULL;
	}
	if (m_pSpeakerLabelIcon)
	{
		delete m_pSpeakerLabelIcon;
		m_pSpeakerLabelIcon = NULL;
	}
	if (m_pScoreboardNeverSpoken)
	{
		delete m_pScoreboardNeverSpoken;
		m_pScoreboardNeverSpoken = NULL;
	}
	if (m_pScoreboardNotSpeaking)
	{
		delete m_pScoreboardNotSpeaking;
		m_pScoreboardNotSpeaking = NULL;
	}
	if (m_pScoreboardSpeaking)
	{
		delete m_pScoreboardSpeaking;
		m_pScoreboardSpeaking = NULL;
	}
	if (m_pScoreboardSpeaking2)
	{
		delete m_pScoreboardSpeaking2;
		m_pScoreboardSpeaking2 = NULL;
	}
	if (m_pScoreboardSquelch)
	{
		delete m_pScoreboardSquelch;
		m_pScoreboardSquelch = NULL;
	}
	if (m_pScoreboardBanned)
	{
		delete m_pScoreboardBanned;
		m_pScoreboardBanned = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the target client has been banned
// Input  : playerID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVoiceStatus::IsPlayerBlocked(int iPlayer)
{
	char playerID[16];
	if (!gEngfuncs.GetPlayerUniqueID(iPlayer, playerID))
		return false;

	return m_BanMgr.GetPlayerBan(playerID);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the player can't hear the other client due to game rules (eg. the other team)
// Input  : playerID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVoiceStatus::IsPlayerAudible(int iPlayer)
{
	/*if (gHUD.m_iGameType > GT_TEAMPLAY)
	{
		if (g_PlayerExtraInfo[iPlayer].teamnumber != 
	}*/
	return !!m_AudiblePlayers[iPlayer];// XDM3033: -1
	//return !!m_AudiblePlayers[iPlayer-1];
}

//-----------------------------------------------------------------------------
// Purpose: blocks/unblocks the target client from being heard
// Input  : playerID - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVoiceStatus::SetPlayerBlockedState(int iPlayer, bool blocked)
{
	if (m_pCvarClientDebug->value > 0.0f)
	{
		gEngfuncs.pfnConsolePrint( "CVoiceStatus::SetPlayerBlockedState part 1\n" );
	}

	char playerID[16];
	if (!gEngfuncs.GetPlayerUniqueID(iPlayer, playerID))
		return false;// XDM

	if (m_pCvarClientDebug->value > 0.0f)
	{
		gEngfuncs.pfnConsolePrint( "CVoiceStatus::SetPlayerBlockedState part 2\n" );
	}

	// Squelch or (try to) unsquelch this player.
	if (m_pCvarClientDebug->value > 0.0f)
	{
		char str[256];
		_snprintf(str, 256, "CVoiceStatus::SetPlayerBlockedState: setting player %d ban to %d\n", iPlayer, !m_BanMgr.GetPlayerBan(playerID));
		gEngfuncs.pfnConsolePrint(str);
	}

	m_BanMgr.SetPlayerBan( playerID, blocked );
	UpdateServerState(false);
	return true;// XDM
}
