#include "stdafx.h"
#include "sprview.h"

#define		ReadBuffer(_file, _dta, _dSize)		((long)fread((_dta), 1, (_dSize), (_file)) == (long)(_dSize))
#define		ReadStruct(_file, _dta)				ReadBuffer((_file), &(_dta), sizeof(_dta))

void TSpriteFrame::ClearFrame(void)
{
	memset(&frame, 0, sizeof(frame));
	memset(&frameType, 0, sizeof(frameType));

	data = NULL;
	dataSize = 0;
	
	next = NULL;
}

void TSpriteFrame::DisposeFrame(void)
{
	if (next)
	{
		delete next;
		next = NULL;
	}

	if (data)
		free(data);

	ClearFrame();
}

void TSpriteFrame::BlastBitmapToPort(HWND theWnd, TSpriteData *sprite)
{
	DirectDrawCurrentFrame(theWnd, this, sprite);
}

BOOL TSpriteFrame::ReadSingleFrame(FILE *iFile)
{
	BOOL isOK;

	if (data)
	{
		free(data);
		data = NULL;
	}
	dataSize = 0;

	isOK = ReadStruct(iFile, frame);
	if (isOK)
	{
		long mxSide = max(frame.width, frame.height);
		
		readSize = (frame.width * frame.height);

		dataSize = mxSide * mxSide;
		data = (unsigned char *)malloc(dataSize);
		if (!data)
			isOK = FALSE;
		else
			isOK = ReadBuffer(iFile, data, readSize);
	}

	return isOK;
}

void TSpriteFrame::WriteFrameToFile(char *fileName, TSpriteData *theSprite)
{
	WriteBMPfile(fileName, data, frame.width, frame.height, theSprite->palette);
}

void TSpriteData::ClearSprite(void)
{
	memset(&header, 0, sizeof(dsprite_t));
	RGBPalette = NULL;
	palette = NULL;
	paletteSize = 0;
	frames = NULL;
	frameCount = 0;
	mxWidth = mxHeight = 0;
	frameBuffer = 0;
}

void TSpriteData::DisposeSprite(void)
{
	if (RGBPalette)
		free(RGBPalette);
	if (palette)
		free(palette);
	if (frames)
		delete frames;
	if (frameBuffer)
		free(frameBuffer);

	ClearSprite();
}

void TSpriteData::CreateRGBPalette(void)
{
	long Cnt;
	BYTE *tPal = palette;

	RGBPalette = (PALETTEENTRY *)malloc(sizeof(PALETTEENTRY) * paletteSize);
	for (Cnt = 0; Cnt < paletteSize; Cnt++)
	{
		RGBPalette[Cnt].peRed	= *tPal++;
		RGBPalette[Cnt].peGreen = *tPal++;
		RGBPalette[Cnt].peBlue	= *tPal++;
		RGBPalette[Cnt].peFlags = PC_NOCOLLAPSE;
	}
}

void TSpriteData::AddFrameToListing(TSpriteFrame *aFrame)
{
	TSpriteFrame *newFrame, *tempFrame;

	newFrame = new TSpriteFrame();
	if (newFrame)
	{
		*newFrame = *aFrame;

		aFrame->data = NULL;
		aFrame->next = NULL;

		if (!frames)
			frames = newFrame;
		else
		{
			tempFrame = frames;
			while (tempFrame->next)
				tempFrame = tempFrame->next;
			tempFrame->next = newFrame;
		}

		frameCount++;
	}
}

BOOL TSpriteData::ReadSprite(FILE *iFile)
{
	BOOL				isOK = TRUE;
	long				Cnt;
	TSpriteFrame		tempFrame;

	DisposeSprite();

	isOK = ReadStruct(iFile, header);

	if (isOK)
		isOK = (header.ident == IDSPRITEHEADER);

	if (isOK)
		isOK = ReadStruct(iFile, paletteSize);

	if (isOK)
	{
		palette = (BYTE *)malloc(paletteSize * 3);
		if (!palette)
			isOK = FALSE;
		else
		{
			isOK = ReadBuffer(iFile, palette, paletteSize * 3);
			if (isOK)
				CreateRGBPalette();
		}
	}

	if (isOK)
	{
		for (Cnt = 0; Cnt < header.numframes; Cnt++)
		{
			if (!isOK)
				break;

			tempFrame.ClearFrame();

			isOK = ReadStruct(iFile, tempFrame.frameType.type);
			if (isOK)
			{
				if (tempFrame.frameType.type == SPR_SINGLE)
					isOK = tempFrame.ReadSingleFrame(iFile);
				else
				{
				}

				if (isOK)
					AddFrameToListing(&tempFrame);
			}

			tempFrame.ClearFrame();
		}
	}

	if (isOK)
	{
		frameBuffer = (char *)malloc(mxWidth * mxHeight);
	}

	if (!isOK)
		DisposeSprite();
	else
	{
		mxWidth = header.width;
		mxHeight = header.height;
	}

	return isOK;
}

TSpriteData *ReadSpriteObject(char *fileName)
{
	TSpriteData *sprite;

	if (strlen(fileName) == 0)
		return NULL;

	sprite = new TSpriteData();
	if (sprite)
	{
		FILE *iFile;

		iFile = fopen(fileName, "rb");
		if (iFile)
		{
			sprite->ReadSprite(iFile);
			fclose(iFile);
		}
		else
		{
			delete sprite;
			sprite = NULL;
		}
	}

	return sprite;
}

TSpriteData *gCurrentSprite = NULL;
TSpriteFrame *gCurrentFrame = NULL;

void ReadCurrentSprite(char *fileName)
{
	if (gCurrentSprite)
	{
		delete gCurrentSprite;
		gCurrentSprite = NULL;
	}

	if (fileName)
	{
		gCurrentSprite = ReadSpriteObject(fileName);
		if (gCurrentSprite)
			gCurrentFrame = gCurrentSprite->frames;
	}
}

void CycleFrame(HWND hwnd)
{
	if (gCurrentSprite)
	{
		if (!gCurrentFrame)
			gCurrentFrame = gCurrentSprite->frames;

		if (gCurrentFrame)
			gCurrentFrame = gCurrentFrame->next;

		InvalidateRect(hwnd, NULL, FALSE);
		UpdateWindow(hwnd);
	}
}

void DisplayFrame(HWND theWnd, HDC theDC, BOOL increment)
{
	if (gCurrentSprite)
	{
		if (!gCurrentFrame)
			gCurrentFrame = gCurrentSprite->frames;

		if (gCurrentFrame)
		{
			gCurrentFrame->BlastBitmapToPort(theWnd, gCurrentSprite);

			if (increment)
			{
				if ((!globalData.playOnce) || (gCurrentFrame->next))
					gCurrentFrame = gCurrentFrame->next;
			}
		}
	}
}

void DisposeCurrentSprite(void)
{
	if (gCurrentSprite)
	{
		delete gCurrentSprite;
		gCurrentSprite = NULL;
	}
	gCurrentFrame = NULL;
}

// Remove quotes from sprite name
void FixSpriteName(char *sName)
{
	long Cnt, lLen;

	if (*sName == '"')
	{
		lLen = strlen(sName);
		for (Cnt = 0; Cnt < lLen; Cnt++)
			sName[Cnt] = sName[Cnt+1];
	}

	lLen = strlen(sName);
	if (lLen > 0)
	{
		if (sName[lLen-1] == '"')
			sName[lLen-1] = 0;
	}
}