/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "hud.h"
#include "cl_util.h"
#include "demo.h"
#include "demo_api.h"
#include <memory.h>
#include "Exports.h"// HL20130901

int g_demosniper = 0;
int g_demosniperdamage = 0;
float g_demosniperorg[3];
float g_demosniperangles[3];
float g_demozoom;

// FIXME:  There should be buffer helper functions to avoid all of the *(int *)& crap.

/*
=====================
Demo_WriteBuffer

Write some data to the demo stream
=====================
*/
void Demo_WriteBuffer( int type, int size, unsigned char *buffer )
{
	//unsigned char buf[ 32 * 1024 ]; warning C6262: Function uses '32784' bytes of stack: exceeds /analyze:stacksize'16384'. Consider moving some data to heap
	unsigned char buf[16384-(sizeof(int)*CHAR_BIT)];// XDM3038c: avoid potential stack overflow. This function is never used with large buffers anyway
	int pos = 0;
	*( int * )&buf[pos] = type;
	pos+=sizeof(int);

	memcpy( &buf[pos], buffer, size );

	// Write full buffer out
	gEngfuncs.pDemoAPI->WriteBuffer( size + sizeof( int ), buf );
}

/*
=====================
Demo_ReadBuffer

Engine wants us to parse some data from the demo stream
=====================
*/
void CL_DLLEXPORT Demo_ReadBuffer( int size, unsigned char *buffer )
{
	DBG_CL_PRINT("Demo_ReadBuffer(%d)\n", size);
//	RecClReadDemoBuffer(size, buffer);

	int i = 0;
	int type = *( int * )buffer;
	i += sizeof( int );
	switch ( type )
	{
	case TYPE_SNIPERDOT:
		g_demosniper = *(int * )&buffer[ i ];
		i += sizeof( int );
		
		if ( g_demosniper )
		{
			g_demosniperdamage = *( int * )&buffer[ i ];
			i += sizeof( int );
			g_demosniperangles[ 0 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperangles[ 1 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperangles[ 2 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperorg[ 0 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperorg[ 1 ] = *(float *)&buffer[i];
			i += sizeof( float );
			g_demosniperorg[ 2 ] = *(float *)&buffer[i];
			i += sizeof( float );
		}
		break;
	case TYPE_ZOOM:
		g_demozoom = *(float * )&buffer[ i ];
		i += sizeof( float );
		break;
	default:
		gEngfuncs.Con_DPrintf( "Unknown demo buffer type, skipping.\n" );
		break;
	}
}
