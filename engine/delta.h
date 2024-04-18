//====================================================================
//
// Purpose: Delta-related header
// Thanks to Uncle Mike
//
//====================================================================

#ifndef DELTA_H
#define DELTA_H
#ifdef _WIN32
#pragma once
#endif


#define DT_BYTE				0x00000001	// A byte
#define DT_SHORT			0x00000002 	// 2 byte field
#define DT_FLOAT			0x00000004	// A floating point field
#define DT_INTEGER			0x00000008	// 4 byte integer
#define DT_ANGLE			0x00000010	// A floating point angle ( will get masked correctly )
#define DT_TIMEWINDOW_8		0x00000020	// A floating point timestamp, relative to sv.time
#define DT_TIMEWINDOW_BIG	0x00000040	// and re-encoded on the client relative to the client's clock
#define DT_STRING			0x00000080	// A null terminated string, sent as 8 byte chars
#define DT_SIGNED			0x00000100	// sign modificator

enum
{
	CUSTOM_NONE = 0,
	CUSTOM_SERVER_ENCODE,	// "gamedll"
	CUSTOM_CLIENT_ENCODE	// "client"
};

// struct info (filled by engine)
/*typedef struct delta_field_s
{
	const char	*name;
	const int	offset;
	const int	size;
} delta_field_t;*/

// one field
typedef struct delta_s
{
	const char	*name;
	int			offset;		// in bytes
	int			size;		// used for bounds checking in DT_STRING
	int			flags;		// DT_INTEGER, DT_FLOAT etc
	float		multiplier;
	float		post_multiplier;	// for DEFINE_DELTA_POST
	int			bits;		// how many bits we send\receive
	qboolean	bInactive;	// unsetted by user request
} delta_t;


#endif // DELTA_H
