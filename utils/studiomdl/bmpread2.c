#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define WIN32_EXTRA_LEAN

#include <stdio.h>
//#include <windows.h>
#include "../../public/archtypes.h"
#include "cmdlib.h"
#include "scriplib.h"
#include "mathlib.h"
#include "../../engine/studio.h"
#include "studiomdl.h"

#ifndef WORD
typedef unsigned short WORD;
#endif

#ifndef DWORD
typedef unsigned long DWORD;
#endif

#ifndef LONG
typedef long			LONG;
#endif

/* constants for the biCompression field */
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

//__declspec(align(1))
#pragma pack(1)
typedef struct tagRGBQUAD {
        byte    rgbBlue;
        byte    rgbGreen;
        byte    rgbRed;
        byte    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;
#pragma pack()

//inline uint16 BitMerge2x1(byte byte1, byte byte2)							{ return (byte1 << 0) | (byte2 << CHAR_BIT); }

//unsigned long ULONG_PTR;
//#define MAKEWORD(a, b)      ((WORD)(((byte)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((byte)(((DWORD_PTR)(b)) & 0xff))) << 8))

// pointer to a simple unallocated pointer
// pRGBAPalette256 same
int ReadBMP3(char *szFile, byte **ppbBits, rgb_t **pRGBAPalette256, int *width, int *height)
{
	int i, rc = 0;
	FILE *pfile = NULL;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD rgrgbPalette[256];
	unsigned long cbBmpBits = 0;
	byte *pbBmpBits = NULL;
	byte *pbReadBits = NULL;
	unsigned long cbPalBytes = 0;
	unsigned long biTrueWidth = 0;

	if (szFile == NULL || *ppbBits != NULL || pRGBAPalette256 == NULL)
		{ printf("invalid BMP file\n"); rc = -1000; goto GetOut; }

	if ((pfile = fopen(szFile, "rb")) == NULL)
		{ printf("unable to open BMP file\n"); rc = -1; goto GetOut; }

	memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	memset(rgrgbPalette, 0, sizeof(RGBQUAD)*256);

	if (fread(&bmfh, sizeof(bmfh), 1/*count*/, pfile) != 1)
		{ rc = -2; goto GetOut; }

	if (!(bmfh.bfReserved1 == 0 && bmfh.bfReserved2 == 0))// Bogus file header check
		{ rc = -2000; goto GetOut; }

	if (fread(&bmih, sizeof bmih, 1/*count*/, pfile) != 1)// Read info header
		{ rc = -3; goto GetOut; }

	if (!(bmih.biSize == sizeof bmih && bmih.biPlanes == 1))// Bogus info header check
	{
		printf("invalid BMP file header\n");
		rc = -4;
		goto GetOut;
	}

	if (bmih.biBitCount != 8)// Bogus bit depth?  Only 8-bit supported.
	{
		printf("BMP file not 8 bit\n");
		rc = -5;
		goto GetOut;
	}

	if (bmih.biCompression != BI_RGB)// Bogus compression?  Only non-compressed supported.
		{ printf("invalid BMP compression type\n"); rc = -6; goto GetOut; }

	if (bmih.biClrUsed == 0)// Figure out how many entires are actually in the table
	{
		bmih.biClrUsed = 256;
		cbPalBytes = (1 << bmih.biBitCount) * sizeof( RGBQUAD );
	}
	else 
		cbPalBytes = bmih.biClrUsed * sizeof( RGBQUAD );

	// Read palette (bmih.biClrUsed entries)
	if (fread(rgrgbPalette, cbPalBytes, 1/*count*/, pfile) != 1)
		{ rc = -6; goto GetOut; }

	*pRGBAPalette256 = (rgb_t *)malloc(sizeof(rgb_t)*256);

	for (i = 0; i < (int)bmih.biClrUsed; i++)// Copy over used entries
	{
		(*pRGBAPalette256)[i].r = rgrgbPalette[i].rgbRed;
		(*pRGBAPalette256)[i].g = rgrgbPalette[i].rgbGreen;
		(*pRGBAPalette256)[i].b = rgrgbPalette[i].rgbBlue;
	}
	for (i = bmih.biClrUsed; i < 256; i++)// unused entires 0,0,0
	{
		(*pRGBAPalette256)[i].r = 0;
		(*pRGBAPalette256)[i].g = 0;
		(*pRGBAPalette256)[i].b = 0;
	}

	// Read bitmap bits (remainder of file)
	cbBmpBits = bmfh.bfSize - ftell(pfile);
	pbReadBits = (unsigned char *)malloc(cbBmpBits);
	memset(pbReadBits, 0, cbBmpBits);
	if (fread(pbReadBits, cbBmpBits, 1/*count*/, pfile) != 1)
	{
		rc = -7;
		free(*pRGBAPalette256);
		goto GetOut;
	}

	pbBmpBits = (unsigned char *)malloc(cbBmpBits);
	memset(pbBmpBits, 0, cbBmpBits);

	biTrueWidth = (bmih.biWidth + 3) & ~3;// data is actually stored with the width being rounded up to a multiple of 4

	// reverse the order of the data
	// (vertical flip & copy)
	for(i = 0; i < bmih.biHeight; i++)
	{
		// when copying, we need to ignore redundant pixels!
		memcpy(&pbBmpBits[i*bmih.biWidth], &pbReadBits[biTrueWidth*(bmih.biHeight-1-i)], bmih.biWidth);
	}

	*width = (WORD)bmih.biWidth;
	*height = (WORD)bmih.biHeight;
	*ppbBits = pbBmpBits;

GetOut:
	if (pfile)
	{
		fclose(pfile);
		pfile = NULL;
	}
	if (pbReadBits)
	{
		free(pbReadBits);
		pbReadBits = NULL;
	}
	return rc;
}

/*int WriteBMP(const char *szFile, byte *pbBits, RGBQUAD *pRGBAPalette256, int width, int height)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	int i, rc = 0;
	FILE *pfile = NULL;
	unsigned long cbBmpBits = 0;
	byte *pbBmpBits = NULL;
	byte  *pb = NULL;
	unsigned long cbPalBytes = 0;
	unsigned long biTrueWidth = 0;

	if (szFile == NULL || pRGBAPalette256 == NULL || pbBits == NULL)
		{ rc = -1000; goto GetOut; }

	if ((pfile = fopen(szFile, "wb")) == NULL)
		{ rc = -1; goto GetOut; }

	memset(&bmfh, 0, sizeof(BITMAPFILEHEADER));
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));

	biTrueWidth = ((width + 3) & ~3);
	cbBmpBits = biTrueWidth * height;
	cbPalBytes = 256 * sizeof(RGBQUAD);

	// Bogus file header check
	bmfh.bfType = MAKEWORD('B','M');
	bmfh.bfSize = sizeof bmfh + sizeof bmih + cbBmpBits + cbPalBytes;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof bmfh + sizeof bmih + cbPalBytes;

	// Write file header
	if (fwrite(&bmfh, sizeof bmfh, 1/*count * /, pfile) != 1)
	{
		rc = -2;
		goto GetOut;
	}

	bmih.biSize = sizeof bmih;// Size of structure
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 8;
	bmih.biCompression = BI_RGB;// Only non-compressed supported.
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 256;// !!
	bmih.biClrImportant = 0;

	if (fwrite(&bmih, sizeof bmih, 1/*count* /, pfile) != 1)// Write info header
		{ rc = -3; goto GetOut; }

	cbPalBytes = bmih.biClrUsed * sizeof(RGBQUAD);// Write palette (bmih.biClrUsed entries)

	if (fwrite(pRGBAPalette256, cbPalBytes, 1/*count* /, pfile) != 1)
		{ rc = -6; goto GetOut; }

	pbBmpBits = (unsigned char *)malloc(cbBmpBits);

	pb = pbBits;
	// reverse the order of the data.
	pb += (height - 1) * width;
	for(i = 0; i < bmih.biHeight; i++)
	{
		memmove(&pbBmpBits[biTrueWidth * i], pb, width);
		pb -= width;
	}

	// Write bitmap bits (remainder of file)
	if (fwrite(pbBmpBits, cbBmpBits, 1/*count* /, pfile) != 1)
		{ rc = -7; goto GetOut; }

	free(pbBmpBits);

GetOut:
	if (pfile) 
		fclose(pfile);

	return rc;
}
*/
