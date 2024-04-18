#define _NOENUMQBOOL
//  winspool.lib comdlg32.lib shell32.lib ole32.lib 
#define  WIN32_LEAN_AND_MEAN

#include <windows.h>

extern "C" 
{
	#include "cmdlib.h"
	#include "wadlib.h"
}
#include "qfont.h"

//#define DEFAULT_FONT "Lucida Console"
#define FONT_TAG	6  // Font's are the 6th tag after the TYP_LUMPY base ( 64 )...i.e., type == 70

void *zeromalloc(size_t size)
{
	unsigned char *pbuffer;
	pbuffer = (unsigned char *)malloc(size);
	if (!pbuffer)
	{
		printf("Failed on allocation of %d bytes", size);
		exit(-1);
	}
	memset(pbuffer, 0, size);
	return ( void * )pbuffer;
}

// Set's the palette to full brightness ( 192 ) and set's up palette entry 0 -- black
void Draw_SetupConsolePalette( unsigned char *pal )
{
	unsigned char *pPalette = pal;
	*(short *)pPalette = 3 * 256;
	pPalette += sizeof( short );

	for (int i = 0; i < 256; i++ )
	{
		pPalette[3 * i + 0] = i;
		pPalette[3 * i + 1] = i;
		pPalette[3 * i + 2] = i;
	}

	// Set palette zero correctly
	pPalette[0] = 0;
	pPalette[1] = 0;
	pPalette[2] = 0;
}

// YWB:  Sigh, VC 6.0's global optimizer causes weird stack fixups in release builds.  Disable the globabl optimizer for this function.
#pragma optimize( "g", off )
// Renders TT font into memory dc and creates appropriate qfont_t structure
qfont_t *CreateConsoleFont( char *pszFont, int nPointSize, BOOL bItalic, BOOL bUnderline, BOOL bBold, BOOL bAntiAlias, int *outsize )
{
	HDC hdc;
	HDC hmemDC;
	RECT rc;
	RECT rcChar;
	HFONT fnt, oldfnt;
	HBITMAP hbm, oldbm;
	BITMAPINFO tempbmi;
	BITMAPINFO *pbmi;
	BITMAPINFOHEADER *pbmheader;
	qfont_t *pqf = NULL;
	unsigned char *pqdata;
	unsigned char *pCur;
	unsigned char *pPalette;
	const int startchar = 32;
	int nScans;
	int w = 16;
	int h = 16 - startchar/w;// 16x16 (=256chars) table starts from space character (32)
	int charheight = nPointSize + 5;
	int charwidth = 16;
	int c;

	printf("\n CreateConsoleFont %s, %d\n", pszFont, nPointSize);
	Sleep(1000);

	// Create the font
	fnt = CreateFont(-nPointSize, 0, 0, 0,
		bBold ? FW_HEAVY : FW_MEDIUM,
		bItalic, bUnderline, 0,
		ANSI_CHARSET,// : ANSI_CHARSET, RUSSIAN_CHARSET
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
		bAntiAlias ? ANTIALIASED_QUALITY : PROOF_QUALITY, VARIABLE_PITCH | FF_DONTCARE, pszFont);

	int fullsize = sizeof(qfont_t) - 4 + (512 * w * charwidth) + sizeof(short) + 768 + 64;

	// Store off final size
	*outsize = fullsize;

	pqf = ( qfont_t * )zeromalloc( fullsize );
	pqdata = (unsigned char *)pqf + sizeof( qfont_t ) - 4;
	pPalette = pqdata + ( 256 * w * charwidth );

	// Configure palette
	Draw_SetupConsolePalette( pPalette );

	hdc			= GetDC(NULL);
	hmemDC		= CreateCompatibleDC(hdc);
	rc.top		= 0;
	rc.left		= 0;
	rc.right	= charwidth  * w;
	rc.bottom	= charheight * h;
	hbm			= CreateBitmap(charwidth * w, charheight * h, 1, 1, NULL);
	oldbm		= (HBITMAP)SelectObject(hmemDC, hbm);
	oldfnt		= (HFONT)SelectObject(hmemDC, fnt);

	SetTextColor(hmemDC, 0x00ffffff);
	SetBkMode(hmemDC, TRANSPARENT);

	// Paint black background
	FillRect(hmemDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

	// Draw character set into memory DC
	int i, j;
	int x, y;
	for (j = 0; j < h; j++)
	{
		for (i = 0; i < w; i++)
		{
			x = i * charwidth;
			y = j * charheight;
			c = (char)(startchar + j * w + i);
			// Only draw printable characters, of course
//			if ( isprint(c) && c <= 255 )// XDM: 127
			{
				rcChar.left		= x + 1;
				rcChar.top		= y + 1;
				rcChar.right	= x + charwidth - 1;
				rcChar.bottom	= y + charheight - 1;
				DrawText(hmemDC, (char *)&c, 1, &rcChar, DT_NOPREFIX | DT_LEFT);// set output to hdc for test
			}
		}
	}

	// Now turn the qfont into raw format
	memset(&tempbmi, 0, sizeof(BITMAPINFO));
	pbmheader					= (BITMAPINFOHEADER *)&tempbmi;
	pbmheader->biSize			= sizeof(BITMAPINFOHEADER);
	pbmheader->biWidth			= w * charwidth; 
	pbmheader->biHeight			= -h * charheight; 
	pbmheader->biPlanes			= 1;
	pbmheader->biBitCount		= 1;
	pbmheader->biCompression	= BI_RGB;

	// Find out how big the bitmap is
	nScans = GetDIBits(hmemDC, hbm, 0, h * charheight, NULL, &tempbmi, DIB_RGB_COLORS);
	// Allocate space for all bits
	pbmi = (BITMAPINFO *)zeromalloc(sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD) + pbmheader->biSizeImage);
	memcpy(pbmi, &tempbmi, sizeof(BITMAPINFO));

	unsigned char *bits = (unsigned char *)pbmi + sizeof(BITMAPINFOHEADER) + 2 * sizeof(RGBQUAD); 
	// Now read in bits
	nScans = GetDIBits(hmemDC, hbm, 0, h * charheight, bits, pbmi, DIB_RGB_COLORS);

	int x1, y1;
	if (nScans > 0)
	{
		// Now convert to proper raw format
		// get results from dib
		pqf->height = 256;
		pqf->width = charwidth;
		pqf->rowheight = charheight;
		pqf->rowcount = h;
		pCur = pqdata;

		memset( pCur, 0xFF, w * charwidth * pqf->height );// Set everything to index 255 ( 0xff ) == transparent
		int edge = 1;
		int bestwidth = 0;
		for ( j = 0; j < h; j++ )
		{
			for ( i = 0; i < w; i++ )
			{
				printf("i %d (%d)\tj %d(%d)\tc %d (%c)\n", i, w, j, h, c, c);
//				Sleep(50);
				x = i * charwidth;
				y = j * charheight;
				c = (char)(startchar + j * w + i);
				pqf->fontinfo[c].charwidth = charwidth;
				pqf->fontinfo[c].startoffset = y * w * charwidth + x;
				bestwidth = 0;
				// In this first pass, place the black drop shadow so characters draw ok in the engine against most backgrounds.
				// Make it all transparent for starters
				for (y1 = 0; y1 < charheight; y1++)
				{
					for (x1 = 0; x1 < charwidth; x1++)
					{
						int offset = (y + y1) * w * charwidth + x + x1;
						pCur = pqdata + offset;// Dest
						pCur[0] = 255;// Assume transparent
					}
				}
				// Put black pixels below and to the right of each pixel
				for (y1 = edge; y1 < charheight - edge; y1++)
				{
					for (x1 = 0; x1 < charwidth; x1++)
					{
						int srcoffset;
						int xx0, yy0;
						int offset = (y + y1) * w * charwidth + x + x1;
						pCur = pqdata + offset;// Dest
						for (xx0 = -edge; xx0 <= edge; xx0++)
						{
							for (yy0 = -edge; yy0 <= edge; yy0++)
							{
								srcoffset = (y + y1 + yy0) * w * charwidth + x + x1 + xx0;
								if (bits[srcoffset >> 3] & (1 << (7 - srcoffset & 7)))
									pCur[0] = 32;// Near Black
							}
						}
					}
				}
				// Now copy in the actual font pixels
				for (y1 = 0; y1 < charheight; y1++)
				{
					for (x1 = 0; x1 < charwidth; x1++)
					{
						int offset = (y + y1) * w * charwidth + x + x1;
						pCur = pqdata + offset;// Dest
						if (bits[offset >> 3] & (1 << (7 - offset & 7)))
						{
							if (x1 > bestwidth)
								bestwidth = x1;
							// FIXME:  Enable true palette support in engine?
							pCur[0] = 192;// Full color
						}
					}
				}
				// Space character width
				if (c == 32)
				{
					bestwidth = 8;
				}
				else
				{
					// Small characters needs can be padded a bit so they don't run into each other
					if (bestwidth <= 14)
						bestwidth += 2;
				}
				// Store off width
				pqf->fontinfo[c].charwidth = bestwidth;
			}// i
		}// j
	}// nScans

	free(pbmi);// Free memory bits
	SelectObject(hmemDC, oldfnt);
	DeleteObject(fnt);
	SelectObject(hmemDC, oldbm);
	DeleteObject(hbm);
	DeleteDC(hmemDC);
	ReleaseDC(NULL, hdc);
	return pqf;
}
#pragma optimize( "g", on )


int main(int argc, char* argv[])
{
	int		i;
	int		outsize[3];
	int		pointsize[3] = { 9, 14, 20 };
	BOOL	bItalic = FALSE;
	BOOL	bBold   = FALSE;
	BOOL	bUnderline = FALSE;
	BOOL	bAntiAlias = FALSE;
//	char	destfile[1024];
	char	sz[ 32 ];
	char	*fontname = NULL;//[ 256 ] = "Lucida Console";
//	DWORD	start, end;
	qfont_t *fonts[3];

	printf("makefont.exe Version 2.0u by XDM (%s)\n", __DATE__ );
	Sleep(0);

	for (i=1 ; i<argc ; i++)
	{
		if (!strcmp(argv[i],"-font"))
		{
			//strcpy( fontname, argv[i+1] );
			fontname = argv[i+1];
			i++;
		}
		else if (!strcmp(argv[i],"-pointsizes"))
		{
			if (i + 3 >= argc)
				Error("Makefont:  Insufficient point sizes specified\n");

			pointsize[0] = atoi( argv[i+1] );
			pointsize[1] = atoi( argv[i+2] );
			pointsize[2] = atoi( argv[i+3] );
			i += 3;
		}
		else if (!strcmp(argv[i],"-italic"))
		{
			bItalic = TRUE;
			printf ( "italic set\n");
		}
		else if (!strcmp(argv[i],"-bold"))
		{
			bBold = TRUE;
			printf ( "bold set\n");
		}
		else if (!strcmp(argv[i],"-underline"))
		{
			bUnderline = TRUE;
			printf ( "underline set\n");
		}
		else if (!strcmp(argv[i],"-aalias"))// XDM
		{
			bAntiAlias = TRUE;
			printf ( "antialiasing enabled\n");
		}
		else if ( argv[i][0] == '-' )
		{
			printf("Unknown option \"%s\"", argv[i]);
		}
		else
			break;
	}

	if (fontname == NULL)
		Error("No font name specified!\n");

	if (i != argc - 1)
		Error("usage: makefont [-font \"fontname\"] [-italic] [-underline] [-bold] [-pointsizes sm med lg] outfile");

	printf("Creating %i, %i, and %i point %s fonts\n", pointsize[0], pointsize[1], pointsize[2], fontname );
//	start = timeGetTime();

	// Create the fonts
	for (i = 0 ; i < 3; i++)
		fonts[i] = CreateConsoleFont(fontname, pointsize[i], bItalic, bUnderline, bBold, bAntiAlias, &outsize[i]);

	// Create wad file
/*	strcpy(destfile, argv[argc - 1]);
	StripExtension(destfile);
	DefaultExtension(destfile, ".wad");*/
	printf("Writing to file...\n");
	NewWad("fonts.wad", false );// destfile

	for (i = 0; i < 3; i++)// Add fonts as lumps
	{
		sprintf( sz, "font%i", i );
		AddLump( sz, fonts[i], outsize[i], TYP_LUMPY + FONT_TAG, false );
	}
	WriteWad(3);// Store results as a WAD3

	for (i = 0 ; i < 3; i++)// Clean up memory
		free(fonts[i]);

//	end = timeGetTime();
//	printf("%5.5f seconds elapsed\n", (float)(end - start)/1000.0);
	printf("Done\n");

	Sleep(1000);// Display for a second since it might not be running from command prompt
	return 0;
}
