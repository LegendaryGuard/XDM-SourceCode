#include "stdafx.h"
#include "sprview.h"

LRESULT CALLBACK InfoDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			char tStr[256];

			if (!gCurrentSprite)
				return FALSE;

			SetWindowText(GetDlgItem(hDlg, IDT_SPRITENAME), spriteName);
			
			sprintf(tStr, "%.3f", gCurrentSprite->header.boundingradius);
			SetWindowText(GetDlgItem(hDlg, IDT_RADIUS), tStr);

			sprintf(tStr, "%ld x %ld", gCurrentSprite->header.width, gCurrentSprite->header.height);
			SetWindowText(GetDlgItem(hDlg, IDT_SIZE), tStr);

			sprintf(tStr, "%ld", gCurrentSprite->header.numframes);
			SetWindowText(GetDlgItem(hDlg, IDT_TOTALFRAMES), tStr);

			switch (gCurrentSprite->header.texFormat)
			{
				case SPR_NORMAL:
					strcpy(tStr, "Normal");
					break;
				case SPR_ADDITIVE:
					strcpy(tStr, "Additive");
					break;
				case SPR_INDEXALPHA:
					strcpy(tStr, "Indexed Alpha");
					break;
				case SPR_ALPHTEST:
					strcpy(tStr, "Alpha Test");
					break;
			}
			SetWindowText(GetDlgItem(hDlg, IDT_TEXTURETYPE), tStr);

			switch (gCurrentSprite->header.type)
			{
				case SPR_VP_PARALLEL_UPRIGHT:	strcpy(tStr, "vp_parallel_upright"); break;
				case SPR_FACING_UPRIGHT:		strcpy(tStr, "facing_upright"); break;
				case SPR_VP_PARALLEL:			strcpy(tStr, "vp_parallel"); break;
				case SPR_ORIENTED:				strcpy(tStr, "oriented"); break;
				case SPR_VP_PARALLEL_ORIENTED:	strcpy(tStr, "vp_parallel_oriented"); break;
			}
			SetWindowText(GetDlgItem(hDlg, IDT_TYPE), tStr);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
			}
			break;
		}

/*		case WM_TIMER:
		{
			IDC_FRAMEPROG
			CProgressCtrl::SetRange(1, gCurrentSprite->header.numframes);
			CProgressCtrl::SetPos(gCurrentFrame);
		}*/
	}
    
	return FALSE;
}

