//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VGUI_LOADTGA_H
#define VGUI_LOADTGA_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* not __MINGW32__ */
#endif


#include "VGUI_BitmapTGA.h"


vgui::BitmapTGA* vgui_LoadTGA(char const *pFilename);
vgui::BitmapTGA* vgui_LoadTGANoInvertAlpha(char const *pFilename);


#endif // VGUI_LOADTGA_H
