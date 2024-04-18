// lbmlib.h

typedef unsigned char	UBYTE;

#ifndef _WINDOWS_
typedef short			WORD;
#endif

typedef unsigned short	UWORD;
typedef long			LONG;

int WriteBMPfile (char *szFile, BYTE *pbBits, int width, int height, BYTE *pbPalette);

typedef struct
{
	BYTE	*ppbBits;
	BYTE	*ppbPalette;
	BITMAPFILEHEADER	bmfh;
	BITMAPINFOHEADER	bmih;
} bitmap_struct;

int	LoadBMP(const char *szFile, bitmap_struct *bData);
