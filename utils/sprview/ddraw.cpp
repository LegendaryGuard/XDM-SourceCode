// DDraw.cpp - DirectDraw support
// The below functions show my inexperience with DirectDraw. The bulk of the 
//  below was taken from DirectDraw samples provided by Microsoft in their DX3 
//  SDK, but the rest was mostly trial and error. I'm sure there is an easier 
//  way of doing this (I'm open to suggestions!).
#include "stdafx.h"
#include "sprview.h"
#include "ddraw.h"

static LPDIRECTDRAW            lpDD = NULL;           // DirectDraw object
static LPDIRECTDRAWSURFACE     lpDDSPrimary = NULL;   // DirectDraw primary surface
static LPDIRECTDRAWSURFACE     lpDDSSurf = NULL;      // Offscreen surface
static LPDIRECTDRAWPALETTE     lpDDPal = NULL;        // DirectDraw palette
static LPDIRECTDRAWCLIPPER     lpClipper = NULL;      // clipper for primary

static char *CreateRGB(TSpriteFrame *theFrame, TSpriteData *theSprite, long planes, long lineAdj);

BOOL CreateWorkingSurface(void)
{
	HRESULT			ddrval;
	DDSURFACEDESC   ddsd;

	if (gCurrentSprite)
	{
		if (!lpDDPal)
		{
			ddrval = lpDD->CreatePalette(DDPCAPS_8BIT, gCurrentSprite->RGBPalette, 
				&lpDDPal, NULL); 
			if (ddrval != DD_OK)
				return FALSE;
		}

		if ((lpDDPal) && (lpDDSPrimary))
			lpDDSPrimary->SetPalette(lpDDPal);

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		ddsd.dwHeight = gCurrentSprite->mxHeight;
		ddsd.dwWidth = gCurrentSprite->mxWidth;
		ddrval = lpDD->CreateSurface(&ddsd, &lpDDSSurf, NULL);
		if (ddrval != DD_OK)
			return FALSE;

		if (lpDDPal)
			lpDDSSurf->SetPalette(lpDDPal);
	}

	return TRUE;
}

void DestroyWorkingSurface(void)
{
	if (lpDDSSurf)
	{
		lpDDSSurf->Release();
		lpDDSSurf = NULL;
	}
}

void DisposeDirectDraw(void)
{
	if (lpDD)
	{
		if (lpDDSPrimary)
		{
			lpDDSPrimary->Release();
			lpDDSPrimary = NULL;
		}
		DestroyWorkingSurface();
		if (lpDDPal)
		{
			lpDDPal->Release();
			lpDDPal = NULL;
		}
		lpDD->Release();
		lpDD = NULL;
	}
}

BOOL InitializeDirectDraw(HWND hwnd)
{
    DDSURFACEDESC   ddsd;
    HRESULT         ddrval;
	BOOL			isOK;

	ddrval = DirectDrawCreate(NULL, &lpDD, NULL);
    if (ddrval != DD_OK)
		return FALSE;

	ddrval = lpDD->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
    if (ddrval != DD_OK)
		return FALSE;

	memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    ddrval = lpDD->CreateSurface( &ddsd, &lpDDSPrimary, NULL );
    if (ddrval != DD_OK)
		return FALSE;

    ddrval = lpDD->CreateClipper(0, &lpClipper, NULL );
    if (ddrval != DD_OK)
		return FALSE;
    
    ddrval = lpClipper->SetHWnd(0, hwnd);
    if (ddrval != DD_OK)
		return FALSE;

    ddrval = lpDDSPrimary->SetClipper(lpClipper);
    if (ddrval != DD_OK)
		return FALSE;

	isOK = CreateWorkingSurface();

    return isOK;
}

void ClearCurrentFrame(HWND theWnd)
{
	if ((lpDDSPrimary) && (gCurrentSprite))
	{
		RECT			theRect, destRect;
		POINT			pt;

		pt.x = pt.y = 0;
		ClientToScreen(theWnd, &pt);

		theRect.left = theRect.top = 0;
		theRect.right = theRect.bottom = 1;

		GetClientRect(theWnd, &destRect);
		OffsetRect(&destRect, pt.x, pt.y);
    
		lpDDSPrimary->Blt(&destRect, lpDDSSurf, &theRect, 0, NULL);
	}
}

// This function takes the image data created above and writes it directly to the secondary
//  surface memory, then "Blt"s it to the primary display.
void DirectDrawCurrentFrame(HWND theWnd, TSpriteFrame *theFrame, TSpriteData *theSprite)
{
    HRESULT			ddrval;
	int				pos = 0;
	RECT			theRect, destRect;
	DDSURFACEDESC	dSurf;
	char			*newData;
	POINT			pt;

	if (lpDDSSurf)
	{
		pt.x = pt.y = 0;
		ClientToScreen(theWnd, &pt);

		GetClientRect(theWnd, &theRect);
		theRect.right = theRect.left + theSprite->mxWidth;
		theRect.bottom = theRect.top + theSprite->mxHeight;

		memset(&dSurf, 0, sizeof(dSurf));
		dSurf.dwSize = sizeof(DDSURFACEDESC);
		ddrval = lpDDSSurf->Lock(NULL, &dSurf, DDLOCK_SURFACEMEMORYPTR, NULL);
		if (!ddrval)
		{
			long depth = dSurf.ddpfPixelFormat.dwRGBBitCount;
			long planes = (depth / 8);
			long lineAdj = ((dSurf.dwWidth * planes) - dSurf.lPitch);

			newData = CreateRGB(theFrame, theSprite, planes, lineAdj);
			if (newData)
			{
				memcpy(dSurf.lpSurface, newData, dSurf.dwWidth * dSurf.dwHeight * planes);
				free(newData);
			}
			lpDDSSurf->Unlock(NULL);

			if (lpDDPal)
			{
				lpDDSSurf->SetPalette(lpDDPal);
				lpDDSPrimary->SetPalette(lpDDPal);
			}

			GetClientRect(theWnd, &destRect);
			destRect.right = destRect.left + (long)((double)theSprite->mxWidth * globalData.magnification);
			destRect.bottom = destRect.top + (long)((double)theSprite->mxHeight * globalData.magnification);
			OffsetRect(&destRect, pt.x, pt.y);

			ddrval = lpDDSPrimary->Blt(&destRect, lpDDSSurf, &theRect, 0, NULL );
		}
	}
}

// THIS function is the wild one... Create a device-depth bitmap from an indexed 8-bit bitmap
//  Most noticeable problems are with 8-bit and 16-bit color depth, where the colors might not
//  match depending on the current driver. 24-bit and 32-bit work great (who uses anything
//  less these days anyway?)
// DirectX MUST have some function or utility to do this sort of thing a lot more easily!
static char *CreateRGB(TSpriteFrame *theFrame, TSpriteData *theSprite, long planes, long lineAdj)
{
	char	*newData;
	long	dSize;
	long	pos1;

	pos1 = 0;

	dSize = max(theFrame->dataSize, theFrame->readSize) + 128;
	dSize = max(dSize, theSprite->mxWidth * theSprite->mxHeight);

	newData = (char *)malloc(dSize * (planes + 1));
	if (newData)
	{
		long Cnt, yCnt, xCnt;

		memset(newData, 0x00, dSize * (planes + 1));

		Cnt = 0;
		for (yCnt = 0; yCnt < theFrame->frame.height; yCnt++)
		{
			for (xCnt = 0; xCnt < theFrame->frame.width; xCnt++)
			{
				PALETTEENTRY pVal;
				BYTE cClr = theFrame->data[Cnt++];

				pVal = theSprite->RGBPalette[cClr];

				switch (planes)
				{
					case 1:	// 8-bit
					{
						newData[pos1++] = cClr;
						break;
					}

					case 2:	// 16-bit
					{
						unsigned long nVal;

						nVal = (pVal.peRed >> 3);
						nVal = (nVal << 6) + (pVal.peGreen >> 2);
						nVal = (nVal << 5) + (pVal.peBlue >> 3);

						newData[pos1++] = (BYTE)(nVal & 0xFF);
						newData[pos1++] = (BYTE)((nVal >> 8) & 0xFF);
						break;
					}

					case 3:	// 24-bit "true color"
					{
						newData[pos1++] = pVal.peBlue;
						newData[pos1++] = pVal.peGreen;
						newData[pos1++] = pVal.peRed;
						break;
					}

					case 4:	// 32-bit "true color"
					{
						newData[pos1++] = pVal.peBlue;
						newData[pos1++] = pVal.peGreen;
						newData[pos1++] = pVal.peRed;
						newData[pos1++] = 0;
						break;
					}
				}
			}

			pos1 += abs(lineAdj);
			if (theFrame->frame.width < theSprite->mxWidth)
				pos1 += (theSprite->mxWidth - theFrame->frame.width) * planes;
		}
	}

	return newData;
}