#include "stdafx.h"
#include "sprview.h"

LRESULT CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
//			SetWindowText(GetDlgItem(hDlg, IDC_VERSIONLINE), "-- Half-Life Rulez! --");
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
	}
    
	return FALSE;
}