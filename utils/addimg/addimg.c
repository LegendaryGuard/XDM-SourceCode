/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

#define VERSION "2.3"
#include "addimg.h"


#define MAXLUMP		0x60000         // XDM: biggest possible lump ++

//extern char qproject[];

int				grabbed;

byte            *byteimage, *lbmpalette;
int              byteimagewidth, byteimageheight;

char            basepath[1024];
char            lumpname[16];

char			destfile[1024];

byte            *lumpbuffer, *lump_p;

qboolean		savesingle;
qboolean		outputcreated;

qboolean		do16bit;
qboolean		fTransparent255;

/*
=============================================================================

							MAIN

=============================================================================
*/

void GrabRaw (void);
void GrabPalette (void);
void GrabPic (void);
void GrabMip (void);
void GrabMip2(int xl, int yl, int w, int h);// XDM: NO FUCKEN TOKENS!!
void GrabColormap (void);
void GrabColormap2 (void);
void GrabFont( void );

typedef struct
{
	char    *name;
	void    (*function) (void);
} command_t;

command_t       commands[] =
{
	{ "palette", GrabPalette },
	{ "colormap", GrabColormap },
	{ "qpic", GrabPic },
	{ "miptex", GrabMip },
	{ "raw", GrabRaw },

	{ "colormap2", GrabColormap2 },
	{ "font", GrabFont },

	{ NULL, NULL }                     // list terminator
};


#define TRANSPARENT_R		0x0
#define TRANSPARENT_G		0x0
#define TRANSPARENT_B		0xFF
#define IS_TRANSPARENT(p)	(p[0]==TRANSPARENT_R && p[1]==TRANSPARENT_G && p[2]==TRANSPARENT_B)
/*
==============
TransparentByteImage
==============
*/
void TransparentByteImage( void )
{
	// Remap all pixels of color 0,0,255 to index 255 and remap index 255 to something else
	byte	transtable[256], *image;
	int		i, j, firsttrans;

	firsttrans = -1;
	for ( i = 0; i < 256; i++ ) {
		if ( IS_TRANSPARENT( (lbmpalette+(i*3)) ) ) {
			transtable[i] = 255;
			if ( firsttrans < 0 )
				firsttrans = i;
		}
		else
			transtable[i] = i;
	}

	// If there is some transparency, translate it
	if ( firsttrans >= 0 ) {
		if ( !IS_TRANSPARENT( (lbmpalette+(255*3)) ) )
			transtable[255] = firsttrans;
		image = byteimage;
		for ( j = 0; j < byteimageheight; j++ ) {
			for ( i = 0; i < byteimagewidth; i++ ) {
				*image = transtable[*image];
				image++;
			}
		}
		// Move palette entry for pixels previously mapped to entry 255
		lbmpalette[ firsttrans*3 + 0 ] = lbmpalette[ 255*3 + 0 ];
		lbmpalette[ firsttrans*3 + 1 ] = lbmpalette[ 255*3 + 1 ];
		lbmpalette[ firsttrans*3 + 2 ] = lbmpalette[ 255*3 + 2 ];
		lbmpalette[ 255*3 + 0 ] = TRANSPARENT_R;
		lbmpalette[ 255*3 + 1 ] = TRANSPARENT_G;
		lbmpalette[ 255*3 + 2 ] = TRANSPARENT_B;
	}
}



/*
==============
LoadScreen
==============
*/
void LoadScreen (char *name)
{
	char	*expanded;

	expanded = ExpandPathAndArchive (name);

	printf ("grabbing from %s...\n",expanded);
	LoadLBM (expanded, &byteimage, &lbmpalette);

	byteimagewidth = bmhd.w;
	byteimageheight = bmhd.h;
}


/*
==============
LoadScreenBMP
==============
*/
void LoadScreenBMP(char *pszName)
{
	char	*pszExpanded;
	char	basename[64];
	
	pszExpanded = ExpandPathAndArchive(pszName);

	printf("grabbing from %s...\n", pszExpanded);
	if (LoadBMP(pszExpanded, &byteimage, &lbmpalette))
		Error ("Failed to load!", pszExpanded);

	if ( byteimage == NULL || lbmpalette == NULL )
		Error("FAIL!",pszExpanded);
	byteimagewidth = bmhd.w;
	byteimageheight = bmhd.h;

	ExtractFileBase (token, basename);		// Files that start with '$' have color (0,0,255) transparent,
	if ( basename[0] == '{' ) {				// move to last palette entry.
		fTransparent255 = true;
		TransparentByteImage();
	}
}


/*
================
CreateOutput
================
*/
void CreateOutput (void)
{
	outputcreated = true;
//
// create the output wadfile file
//
	NewWad (destfile, false);	// create a new wadfile
}

/*
===============
WriteLump
===============
*/
void WriteLump (int type, int compression)
{
	int		size;
	
	if (!outputcreated)
		CreateOutput ();

//
// dword align the size
//
	while ((int)lump_p&3)
		*lump_p++ = 0;

	size = lump_p - lumpbuffer;
	if (size > MAXLUMP)
		Error ("Lump size exceeded %d, memory corrupted!",MAXLUMP);

//
// write the grabbed lump to the wadfile
//
	AddLump (lumpname,lumpbuffer,size,type, compression);
}

/*
===========
WriteFile

Save as a seperate file instead of as a wadfile lump
===========
*/
void WriteFile (void)
{
	char	filename[1024];
	char	*exp;

	sprintf (filename,"%s/%s.lmp", destfile, lumpname);
	exp = ExpandPath(filename);
	printf ("saved %s\n", exp);
	SaveFile (exp, lumpbuffer, lump_p-lumpbuffer);		
}

/*
================
ParseScript
================
*/
void ParseScript (void)
{
	int			cmd;
	int			size;

	fTransparent255 = false;
	do
	{
		//
		// get a command / lump name
		//
		GetToken (true);
		if (endofscript)
			break;
		if (!Q_strcasecmp (token,"$LOAD"))
		{
			GetToken (false);
			LoadScreen (token);
			continue;
		}

		if (!Q_strcasecmp (token,"$DEST"))
		{
			GetToken (false);
			strcpy (destfile, token);
			continue;
		}

		if (!Q_strcasecmp (token,"$SINGLEDEST"))
		{
			GetToken (false);
			strcpy (destfile, token);
			savesingle = true;
			continue;
		}


		if (!Q_strcasecmp (token,"$LOADBMP"))
		{
			GetToken (false);
			fTransparent255 = false;
			LoadScreenBMP (token);
			continue;
		}

		//
		// new lump
		//
		if (strlen(token) >= sizeof(lumpname) )
			Error ("\"%s\" is too long to be a lump name",token);
		memset (lumpname,0,sizeof(lumpname));			
		strcpy (lumpname, token);
		for (size=0 ; size<sizeof(lumpname) ; size++)
			lumpname[size] = tolower(lumpname[size]);

		//
		// get the grab command
		//
		lump_p = lumpbuffer;

		GetToken (false);

		//
		// call a routine to grab some data and put it in lumpbuffer
		// with lump_p pointing after the last byte to be saved
		//
		for (cmd=0 ; commands[cmd].name ; cmd++)
			if ( !Q_strcasecmp(token,commands[cmd].name) )
			{
				commands[cmd].function ();
				break;
			}

		if ( !commands[cmd].name )
			Error ("Unrecognized token '%s' at line %i",token,scriptline);
	
		grabbed++;
		
		if (savesingle)
			WriteFile ();
		else	
			WriteLump (TYP_LUMPY+cmd, 0);
		
	} while (!endofscript);
}

extern outwad, wadhandle;

int main(int argc, char **argv)
{
	int		i, size;
	int			cmd;

	printf("addimg by xawari (%s)\n", __DATE__);

	if (argc < 3)
		Error("addimg <WAD file name> <BMP file name> [BMP file name ...]");

	lumpbuffer = malloc (MAXLUMP);
	do16bit = true;

	W_OpenWad(argv[1]);
	outwad = wadhandle;
	outputcreated = true;

	for (i=2; i<argc; i++)
	{
		if (strlen(argv[i]) >= sizeof(lumpname))
			printf("WARNING: \"%s\" is too long to be a lump name", argv[i]);

		memset(lumpname, 0, sizeof(lumpname));			
		strcpy(lumpname, argv[i]);

		for (size = 0; size<sizeof(lumpname); size++)
			lumpname[size] = tolower(lumpname[size]);

		lump_p = lumpbuffer;

		LoadScreenBMP(argv[i]);
		StripFilename(argv[i]);

		GrabMip2(-1,-1,-1,-1);

		WriteLump(TYP_LUMPY+3, 0);// 3 is miptex
	}

	getch();
	return 0;
}
