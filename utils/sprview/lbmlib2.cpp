// These functions are copied from the "lbmlib.c" file provided by Valve in their
// "standard SDK", but the LBM support code was removed and the "LoadBMP" function
// was redefined to return all the data in one structure. Makes handling the data
// much more easier.
// Also, some of the defined were changed (from "byte" to "BYTE", for instance),
// and made it a "cpp" file rather than a "c" file.
// Thanks Valve for saving me from writing these!

#include "stdafx.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "lbmlib2.h"

int LoadBMP(const char* szFile, bitmap_struct *bData)
{
	int i, rc = 0;
	FILE *pfile = NULL;
	RGBQUAD rgrgbPalette[256];
	ULONG cbBmpBits;
	BYTE* pbBmpBits;
	BYTE  *pb, *pbPal = NULL;
	ULONG cbPalBytes;
	ULONG biTrueWidth;

	// Bogus parameter check
	if (!bData)
		{ fprintf(stderr, "invalid BMP file\n"); rc = -1000; goto GetOut; }

	memset(bData, 0, sizeof(bitmap_struct));

	// File exists?
	if ((pfile = fopen(szFile, "rb")) == NULL)
		{ /*fprintf(stderr, "unable to open BMP file\n");*/ rc = -1; goto GetOut; }
	
	// Read file header
	if (fread(&(bData->bmfh), sizeof(BITMAPFILEHEADER), 1/*count*/, pfile) != 1)
		{ rc = -2; goto GetOut; }

	// Bogus file header check
	if (!(bData->bmfh.bfReserved1 == 0 && bData->bmfh.bfReserved2 == 0))
		{ rc = -2000; goto GetOut; }

	// Read info header
	if (fread(&(bData->bmih), sizeof(BITMAPINFOHEADER), 1/*count*/, pfile) != 1)
		{ rc = -3; goto GetOut; }

	// Bogus info header check
	if (!(bData->bmih.biSize == sizeof(BITMAPINFOHEADER) && bData->bmih.biPlanes == 1))
		{ fprintf(stderr, "invalid BMP file header\n");  rc = -3000; goto GetOut; }

	// Bogus bit depth?  Only 8-bit supported.
	if (bData->bmih.biBitCount != 8)
		{ fprintf(stderr, "BMP file not 8 bit\n");  rc = -4; goto GetOut; }
	
	// Bogus compression?  Only non-compressed supported.
	if (bData->bmih.biCompression != BI_RGB)
		{ fprintf(stderr, "invalid BMP compression type\n"); rc = -5; goto GetOut; }

	// Figure out how many entires are actually in the table
	if (bData->bmih.biClrUsed == 0)
		{
		bData->bmih.biClrUsed = 256;
		cbPalBytes = (1 << bData->bmih.biBitCount) * sizeof( RGBQUAD );
		}
	else 
		{
		cbPalBytes = bData->bmih.biClrUsed * sizeof( RGBQUAD );
		}

	// Read palette (bmih.biClrUsed entries)
	if (fread(rgrgbPalette, cbPalBytes, 1/*count*/, pfile) != 1)
		{ rc = -6; goto GetOut; }

	// convert to a packed 768 byte palette
	pbPal = (unsigned char *)malloc(768);
	if (pbPal == NULL)
		{ rc = -7; goto GetOut; }

	pb = pbPal;

	// Copy over used entries
	for (i = 0; i < (int)bData->bmih.biClrUsed; i++)
	{
		*pb++ = rgrgbPalette[i].rgbRed;
		*pb++ = rgrgbPalette[i].rgbGreen;
		*pb++ = rgrgbPalette[i].rgbBlue;
	}

	// Fill in unused entires will 0,0,0
	for (i = bData->bmih.biClrUsed; i < 256; i++) 
	{
		*pb++ = 0;
		*pb++ = 0;
		*pb++ = 0;
	}

	// Read bitmap bits (remainder of file)
	cbBmpBits = bData->bmfh.bfSize - ftell(pfile);
	pb = (unsigned char *)malloc(cbBmpBits);
	if (fread(pb, cbBmpBits, 1/*count*/, pfile) != 1)
		{ rc = -7; goto GetOut; }

	pbBmpBits = (unsigned char *)malloc(cbBmpBits);

	// data is actually stored with the width being rounded up to a multiple of 4
	biTrueWidth = (bData->bmih.biWidth + 3) & ~3;
	
	// reverse the order of the data.
	pb += (bData->bmih.biHeight - 1) * biTrueWidth;
	for(i = 0; i < bData->bmih.biHeight; i++)
	{
		memmove(&pbBmpBits[biTrueWidth * i], pb, biTrueWidth);
		pb -= biTrueWidth;
	}

	pb += biTrueWidth;
	free(pb);

	bData->ppbPalette = pbPal;
	bData->ppbBits = pbBmpBits;

GetOut:
	if (pfile) 
		fclose(pfile);

	return rc;
}


int WriteBMPfile (char *szFile, BYTE *pbBits, int width, int height, BYTE *pbPalette)
{
	int i, rc = 0;
	FILE *pfile = NULL;
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;
	RGBQUAD rgrgbPalette[256];
	ULONG cbBmpBits;
	BYTE* pbBmpBits;
	BYTE  *pb, *pbPal = NULL;
	ULONG cbPalBytes;
	ULONG biTrueWidth;

	// Bogus parameter check
	if (!(pbPalette != NULL && pbBits != NULL))
		{ rc = -1000; goto GetOut; }

	// File exists?
	if ((pfile = fopen(szFile, "wb")) == NULL)
		{ rc = -1; goto GetOut; }

	biTrueWidth = ((width + 3) & ~3);
	cbBmpBits = biTrueWidth * height;
	cbPalBytes = 256 * sizeof( RGBQUAD );

	// Bogus file header check
	bmfh.bfType = MAKEWORD( 'B', 'M' );
	bmfh.bfSize = sizeof bmfh + sizeof bmih + cbBmpBits + cbPalBytes;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof bmfh + sizeof bmih + cbPalBytes;

	// Write file header
	if (fwrite(&bmfh, sizeof bmfh, 1/*count*/, pfile) != 1)
		{ rc = -2; goto GetOut; }

	// Size of structure
	bmih.biSize = sizeof bmih;
	// Width
	bmih.biWidth = biTrueWidth;
	// Height
	bmih.biHeight = height;
	// Only 1 plane 
	bmih.biPlanes = 1;
	// Only 8-bit supported.
	bmih.biBitCount = 8;
	// Only non-compressed supported.
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;

	// huh?
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;

	// Always full palette
	bmih.biClrUsed = 256;
	bmih.biClrImportant = 0;
	
	// Write info header
	if (fwrite(&bmih, sizeof bmih, 1/*count*/, pfile) != 1)
		{ rc = -3; goto GetOut; }
	

	// convert to expanded palette
	pb = pbPalette;

	// Copy over used entries
	for (i = 0; i < (int)bmih.biClrUsed; i++)
	{
		rgrgbPalette[i].rgbRed   = *pb++;
		rgrgbPalette[i].rgbGreen = *pb++;
		rgrgbPalette[i].rgbBlue  = *pb++;
        rgrgbPalette[i].rgbReserved = 0;
	}

	// Write palette (bmih.biClrUsed entries)
	cbPalBytes = bmih.biClrUsed * sizeof( RGBQUAD );
	if (fwrite(rgrgbPalette, cbPalBytes, 1/*count*/, pfile) != 1)
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
	if (fwrite(pbBmpBits, cbBmpBits, 1/*count*/, pfile) != 1)
		{ rc = -7; goto GetOut; }

	free(pbBmpBits);

GetOut:
	if (pfile) 
		fclose(pfile);

	return rc;
}