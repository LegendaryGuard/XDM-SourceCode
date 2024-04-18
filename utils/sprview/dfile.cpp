#include "stdafx.h"
#include <commdlg.h>
#include "sprview.h"

static LPCSTR szOpenFilter[] = 
{	
	"Half-Life Sprites (*.spr)", "*.spr",
	"All files (*.*)", "*.*",
	""
};

static LPCSTR szSaveFrameFilter[] = 
{	
	"Bitmap files (*.bmp)", "*.bmp",
	""
};

char szPath[kMaxFileName] = "";
static char szFileName[kMaxFileName] = "";
static char szString[kMaxFileName] = "";

static BOOL DoGetOpenFilename(HWND hWnd)
{
	OPENFILENAME	ofn;

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= hWnd;
	ofn.lpstrFilter		= szOpenFilter[0];
	ofn.lpstrFile		= (LPSTR)szPath;
	ofn.nMaxFile		= kMaxFileName;
	ofn.lpstrFileTitle	= (LPSTR)szFileName;
	ofn.nMaxFileTitle	= kMaxFileName;
	ofn.lpstrTitle		= "Open Sprite...";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

	return GetOpenFileName(&ofn);
}

BOOL DoOpenFile(HWND hWnd)
{
	BOOL	isOK;

	strcpy(szPath, "");

	isOK = DoGetOpenFilename(hWnd);
	if (isOK)
	{
		strcpy(spriteName, szPath);

		DestroyWorkingSurface();
		ReadCurrentSprite(spriteName);
		CreateWorkingSurface();
	}

	return isOK;
}

static BOOL DoGetSaveFilename(HWND hWnd, char *theTitle)
{
	OPENFILENAME	ofn;

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= hWnd;
	ofn.lpstrFilter		= szSaveFrameFilter[0];
	ofn.lpstrFile		= (LPSTR)szPath;
	ofn.nMaxFile		= kMaxFileName;
	ofn.lpstrFileTitle	= (LPSTR)szFileName;
	ofn.nMaxFileTitle	= kMaxFileName;
	ofn.lpstrTitle		= theTitle;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_OVERWRITEPROMPT;

	return GetSaveFileName(&ofn);
}

void DoSaveFile(HWND hWnd)
{
	BOOL	isOK;

	if (gCurrentSprite)
	{
		char *sPos;
		
		strcpy(spriteName, szPath);
		sPos = strchr(szPath, '.');
		if (sPos)
		{
			*sPos = 0;
			strcat(szPath, ".bmp");
		}

		isOK = DoGetSaveFilename(hWnd, "Save Frame...");
		if (isOK)
		{
			if (!gCurrentFrame)
				gCurrentFrame = gCurrentSprite->frames;

			if (gCurrentFrame)
			{
				FILE	*qcFile = NULL;
				int		result;
				char	*sPos, tempName[256];

				result = MessageBox(hWnd, "Create \".qc\" script file?", "Save sequence...", 
					MB_YESNOCANCEL);
				if (result == IDCANCEL)
					return;

				if (result == IDYES)
				{
					strcpy(tempName, szPath);
					sPos = strchr(tempName, '.');
					if (sPos)
						*sPos = 0;
					strcat(tempName, ".qc");
					qcFile = fopen(tempName, "wt");
				}

				if (!strchr(szPath, '.'))
					strcat(szPath,".bmp");

				SendQCHeader(qcFile, szPath, gCurrentSprite);

				if (!gCurrentFrame)
					gCurrentFrame = gCurrentSprite->frames;
				gCurrentFrame->WriteFrameToFile(szPath, gCurrentSprite);

				SendQCFrame(qcFile, szPath, gCurrentFrame);

				if (qcFile)
				{
					fprintf(qcFile, "\n\n");
					fclose(qcFile);
				}
			}
		}
	}
}

void DoSaveSequence(HWND hWnd)
{
	BOOL	isOK;

	if (gCurrentSprite)
	{
		isOK = DoGetSaveFilename(hWnd, "Save Sequence...");
		if (isOK)
		{
			FILE	*qcFile = NULL;
			char	*sPos = strchr(szPath, '.');
			long	sCnt = 1;
			char	tempName[256];
			int		result;

			result = MessageBox(hWnd, "Create \".qc\" script file?", "Save sequence...", 
				MB_YESNOCANCEL);
			if (result == IDCANCEL)
				return;

			if (sPos)
				*sPos = 0;

			sprintf(tempName, "%s.qc", szPath);
			if (result == IDYES)
				qcFile = fopen(tempName, "wt");

			SendQCHeader(qcFile, szPath, gCurrentSprite);
			
			gCurrentFrame = gCurrentSprite->frames;
			while (gCurrentFrame)
			{
				sprintf(tempName, "%s%03d.bmp", szPath, sCnt++);
				gCurrentFrame->WriteFrameToFile(tempName, gCurrentSprite);
				SendQCFrame(qcFile, tempName, gCurrentFrame);
				gCurrentFrame = gCurrentFrame->next;
			}

			if (qcFile)
			{
				fprintf(qcFile, "\n\n");
				fclose(qcFile);
			}
			gCurrentFrame = gCurrentSprite->frames;
		}
	}
}