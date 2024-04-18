//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef SPECTATOR_H
#define SPECTATOR_H
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */

#include "cl_entity.h"
#include "interpolation.h"

#define MAX_SPEC_HUD_MESSAGES		8

#define OVERVIEW_MAX_LAYERS			8// XDM
#define OVERVIEW_MAX_ENTITIES		128

#define OVERVIEW_ENTSIZE_DEFAULT	24.0f
#define OVERVIEW_ENTSIZE_PLAYER		32.0f
#define OVERVIEW_ENTSIZE_GAMEGOAL	128.0f

#define	MAX_CAM_WAYPOINTS			32// HL20130901

enum inset_mode_e
{
	INSET_OFF = 0,
	INSET_CHASE_FREE,// WORLD mode
	INSET_IN_EYE,// WORLD mode
	INSET_ROAMING,// WORLD mode without target
	INSET_MAP_FREE,// MAP mode without target
	INSET_MAP_CHASE// MAP mode
};

typedef struct overviewInfo_s
{
	char		map[MAX_MAPPATH];	// cl.levelname or empty
	vec3_t		origin;		// center of map
	float		zoom;		// zoom of map images
	size_t		layers;		// how may layers do we have
	float		layersHeights[OVERVIEW_MAX_LAYERS];
	char		layersImages[OVERVIEW_MAX_LAYERS][256];
	//int		layersRenderModes[OVERVIEW_MAX_LAYERS];// XDM
	int			insetWindowX;
	int			insetWindowY;
	int			insetWindowWidth;// XDM3038a: that was... silly.
	int			insetWindowHeight;
	short		rotated;// are map images rotated (90 degrees) ?
} overviewInfo_t;

typedef struct overviewEntity_s
{
	struct cl_entity_s	*entity;
	float/*double*/		killTime;// XDM3035a: GetClientTime does not offer double precision
	struct model_s		*sprite;
	float				scale;// XDM3038
//	bool				viewport_parallel;// XDM3035c: parallel to viewport
} overviewEntity_t;

// HL20130901
typedef struct cameraWayPoint_s
{
	float	time;
	vec3_t	position;
	vec3_t	angle;
	float	fov;
	int		flags;
} cameraWayPoint_t;


//-----------------------------------------------------------------------------
// Purpose: Handles the drawing of the spectator stuff (camera & top-down map and all the things on it )
//-----------------------------------------------------------------------------
class CHudSpectator : public CHudBase
{
public:
	virtual int Init(void);
	virtual int VidInit(void);
	virtual int Draw(const float &flTime);
	virtual void InitHUDData(void);
	virtual void Reset(void);

	int ToggleInset(int newInsetMode, bool allowOff);
	void CheckSettings(void);
	bool AddOverviewEntity(struct cl_entity_s *ent, struct model_s *icon, float scale, float lifetime);
	bool AddOverviewEntityToList(cl_entity_t *ent, const struct model_s *sprite, float scale, float killTime);
	void DeathMessage(int victim);

	bool DrawOverview(bool bTransparent);
	void DrawOverviewLayer(void);
	void DrawOverviewEntities(void);
	bool OverviewIsFullscreen(void);

	void GetMapPosition(float *returnvec);
	void LoadMapSprites(void);
	bool ParseOverviewFile(void);
	void SetModes(int iMainMode, int iInsetMode);
	void HandleButtonsDown(int ButtonPressed);
	void HandleButtonsUp(int ButtonPressed);
	void FindNextPlayer(bool bReverse);
	void FindPlayer(CLIENT_INDEX iPlayerIndex);
	void SetSpectatorStartPosition(void);
//	void DumpOverviewEnts(void);// XDM
	bool ShouldDrawOverview(void);// XDM
	bool ShouldDrawOverviewWindow(void);// XDM3037a
//	short GetOverviewDrawCycle(void);// XDM3037
	void GetInsetWindowBounds(int *x, int *y, int *w, int *h);
	bool GetAutoDirector(void);
	bool GetShowStatus(void);

	void DirectorMessage(int iSize, void *pbuf);

	void AddWaypoint(float time, const vec3_t &pos, const vec3_t &angle, float fov, int flags);
	void SetCameraView(const vec3_t &pos, const vec3_t &angle, float fov);
	float GetFOV(void);
	bool GetDirectorCamera(vec3_t &position, vec3_t &angle);
	void SetWayInterpolation(cameraWayPoint_t *prev, cameraWayPoint_t *start, cameraWayPoint_t *end, cameraWayPoint_t *next);

	int					m_lastHudMessage;
	int					m_iObserverFlags;
	int					m_iSpectatorNumber;
	//int					m_iProxiesNumber;
	float				m_mapZoom;		// zoom the user currently uses
	int					m_iInsetMode;// XDM
	int					m_ChaseEntity;	// if != 0, follow this entity with viewangles
	int					m_WayPoint;		// current waypoint 1
	int					m_NumWayPoints;	// current number of waypoints
	qboolean			m_chatEnabled;
	qboolean			m_IsInterpolating;
	vec3_t				m_mapOrigin;	// origin where user rotates around
	vec3_t				m_cameraOrigin;	// a help camera
	vec3_t				m_cameraAngles;	// and it's angles
//	Vector				m_vecViewPortAngles;
	short				m_iDrawCycle;
	short				m_OverviewMapDrawPass;// XDM3037a: we're currently drawing overview MAP, not camera!
	CInterpolation		m_WayInterpolation;
	client_textmessage_t	m_HUDMessages[MAX_SPEC_HUD_MESSAGES];
	char				m_HUDMessageText[MAX_SPEC_HUD_MESSAGES][128];

private:
	cvar_t *			m_drawnames;
	cvar_t *			m_drawcone;
	cvar_t *			m_drawstatus;
	cvar_t *			m_autoDirector;
	cvar_t *			m_insetBounds;
	overviewInfo_t		m_OverviewData;
	overviewEntity_t	m_OverviewEntities[OVERVIEW_MAX_ENTITIES];
	vec3_t				m_vPlayerPos[MAX_PLAYERS];// XDM3035: TESTME
	HLSPRITE			m_hsprCamera;
	HLSPRITE			m_hsprPlayer;
	HLSPRITE			m_hsprPlayerDead;
	HLSPRITE			m_hsprGameObject;// XDM3038
	HLSPRITE			m_hsprViewcone;
	HLSPRITE			m_hsprUnkown;// XDM3038
	HLSPRITE			m_hsprUnkownMap;
	HLSPRITE			m_hsprBeam;
	HLSPRITE			m_hCrosshair;
	wrect_t				m_crosshairRect;
	struct model_s		*m_DefaultSprite;// XDM
	struct model_s		*m_MapSprite[OVERVIEW_MAX_LAYERS];// each layer image is saved in one sprite, where each tile is a sprite frame
	float				m_flNextObserverInput;
	float				m_FOV;
	float				m_zoomDelta;
	float				m_moveDelta;
	int					m_lastPrimaryObject;
	int					m_lastSecondaryObject;

	cameraWayPoint_t	m_CamPath[MAX_CAM_WAYPOINTS];
};

#endif // SPECTATOR_H
