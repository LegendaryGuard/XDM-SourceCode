#ifndef		__SPRVIEW_H__
#define		__SPRVIEW_H__

#include "spritegn.h"
#include "resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

class TSpriteData; // Forward reference

// A single animation frame
class TSpriteFrame
{
public:
	TSpriteFrame()		{	ClearFrame();		}
	~TSpriteFrame()		{	DisposeFrame();		}

	dspriteframe_t		frame;
	dspriteframetype_t	frameType;
	unsigned char		*data;
	long				dataSize, readSize;
	TSpriteFrame		*next;
	void ClearFrame(void);
	void DisposeFrame(void);
	BOOL ReadSingleFrame(FILE *iFile);
	void WriteFrameToFile(char *fileName, TSpriteData *theSprite);
	void BlastBitmapToPort(HWND theWnd, TSpriteData *sprite);
};

// The whole sprite. Individual frames are saved in a linked list
class TSpriteData
{
public:
	TSpriteData()		{	ClearSprite();		}
	~TSpriteData()		{	DisposeSprite();	}

	dsprite_t		header;
	PALETTEENTRY	*RGBPalette;
	BYTE			*palette;
	short			paletteSize;
	TSpriteFrame	*frames;
	long			frameCount;
	long			mxWidth, mxHeight;
	char			*frameBuffer;
	BOOL ReadSprite(FILE *iFile);
	void AddFrameToListing(TSpriteFrame *aFrame);
	void BlastFramesToPort(HWND theWnd);
	void CreateRGBPalette(void);

private:
	void ClearSprite(void);
	void DisposeSprite(void);
};

// Current global values
typedef struct
{
	float		magnification;
	long		fps;
	BOOL		isCyclic, playOnce;
} GlobalDataStruct;

extern GlobalDataStruct globalData;

// Currently active sprite name
extern char spriteName[256];

// Information on current frame ("RSprite.cpp")
extern TSpriteData *gCurrentSprite;
extern TSpriteFrame *gCurrentFrame;

// File manipulation ("DFile.cpp")
#define		kMaxFileName		256

extern char szPath[kMaxFileName];
extern BOOL DoOpenFile(HWND hWnd);
extern void DoSaveFile(HWND hWnd);
extern void DoSaveSequence(HWND hWnd);
extern void ReadCurrentSprite(char *fileName);	// "RSprite.cpp"
// Window timer for drawing animated sprite ("RSpeed.cpp")
extern void StartDrawTimer(HWND hWnd);
extern void StopDrawTimer(HWND hWnd);
// DirectDraw support functions ("DDraw.cpp")
extern BOOL InitializeDirectDraw(HWND hwnd);
extern void DisposeDirectDraw(void);
extern void ClearCurrentFrame(HWND theWnd);
extern void DisplayFrame(HWND theWnd, HDC theDC, BOOL increment);
extern void DirectDrawCurrentFrame(HWND theWnd, TSpriteFrame *theFrame, TSpriteData *theSprite);
extern BOOL CreateWorkingSurface(void);
extern void DestroyWorkingSurface(void);
// Bitmap support functions (provided in Valve's "lbmlib.c" and "cmdlib.c")
extern int WriteBMPfile(char *szFile, BYTE *pbBits, int width, int height, BYTE *pbPalette);
// ".qc" file support ("QCFile.cpp")
extern void SendQCHeader(FILE *qcFile, char *spritePath, TSpriteData *theSprite);
extern void SendQCFrame(FILE *qcFile, char *fileName, TSpriteFrame *theFrame);
// Miscellaneous support functions for active sprite
extern void CycleFrame(HWND hwnd);
extern void DisposeCurrentSprite(void);
extern void FixSpriteName(char *sName);
// Windows support functions and variables
extern LRESULT CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK SpeedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK InfoDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
extern HINSTANCE hInst; 

#endif