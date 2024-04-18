#include "stdafx.h"
#include "sprview.h"

#define kDrawTimerID	1051

// This function is what's being called by the main window when the timer is created with "SetTimer".
static VOID CALLBACK DisplaySpriteProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if (gCurrentSprite)
	{
		HDC theDC;
		PAINTSTRUCT ps;

		theDC = BeginPaint(hWnd, &ps);
		DisplayFrame(hWnd, theDC, globalData.isCyclic);
		EndPaint(hWnd, &ps);
	}
}

// These turn on/off the Windows event timer for screen draw
void StartDrawTimer(HWND hWnd)
{
	SetTimer(hWnd, kDrawTimerID, 1000 / globalData.fps, DisplaySpriteProc);
}

void StopDrawTimer(HWND hWnd)
{
	KillTimer(hWnd, kDrawTimerID);
}

LRESULT CALLBACK SpeedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			char tStr[21];
			ltoa(globalData.fps, tStr, 16);
			SetWindowText(GetDlgItem(hDlg, IDC_SPEED), tStr);
			SetFocus(GetDlgItem(hDlg, IDC_SPEED));
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					char tStr[21];
					long tVal;
					GetWindowText(GetDlgItem(hDlg, IDC_SPEED), tStr, 20);
					tVal = atol(tStr);
					if (tVal > 0)
					{
						if (tVal != globalData.fps)
						{
							HWND hParent = GetParent(hDlg);

							globalData.fps = tVal;
							StopDrawTimer(hParent);
							StartDrawTimer(hParent);
						}
					}

					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				}
				
				case IDCANCEL:
				{
					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				}
			}
			break;
		}
	}
	return FALSE;
}
