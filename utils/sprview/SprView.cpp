#include "stdafx.h"
#include "sprview.h"

GlobalDataStruct globalData;
char spriteName[256] = "";

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// The title bar text

// Foward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

// Resizes the main window to the size of the sprite, keeping it within a minimum amount
//  so you could still see the full menu
#define		kDefaultX			250
#define		kDefaultY			100

void ResizeMainWindow(HWND hWnd)
{
	long	xSize, ySize;
	RECT	theRect;

	GetWindowRect(hWnd, &theRect);

	if (gCurrentSprite)
	{
		xSize = (long)((double)gCurrentSprite->mxWidth * globalData.magnification) + 16;
		if (xSize < kDefaultX)
			xSize = kDefaultX;

		ySize = (long)((double)gCurrentSprite->mxHeight * globalData.magnification) + 64;
		if (ySize < kDefaultY)
			ySize = kDefaultY;
	}
	else
	{
		xSize = kDefaultX;
		ySize = kDefaultY;
	}
	
	MoveWindow(hWnd, theRect.left, theRect.top, xSize, ySize, TRUE);
}
   

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	if (strlen(lpCmdLine) > 0)
	{
		strcpy(spriteName, lpCmdLine);
		FixSpriteName(spriteName);
	}

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SPRITEVIEWER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_SPRITEVIEWER);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

//  FUNCTION: MyRegisterClass()
//  PURPOSE: Registers the window class.
//  COMMENTS:
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_LAMBDA_N);
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_LAMBDA_S);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_SPRITEVIEWER;
	wcex.lpszClassName	= szWindowClass;
	return RegisterClassEx(&wcex);
}

//   FUNCTION: InitInstance(HANDLE, int)
//   PURPOSE: Saves instance handle and creates main window
//   COMMENTS:
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   long xSize, ySize;
   DWORD flags;

   if (gCurrentSprite)
   {
	   xSize = gCurrentSprite->mxWidth;
	   ySize = gCurrentSprite->mxHeight;
   }
   else
   {
	   xSize = kDefaultX;
	   ySize = kDefaultY;
   }

   hInst = hInstance; // Store instance handle in our global variable

   flags = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
   hWnd = CreateWindow(szWindowClass, szTitle, flags, CW_USEDEFAULT, 0, xSize, ySize, NULL, NULL, hInstance, NULL);

   if (!hWnd)
      return FALSE;

   ResizeMainWindow(hWnd);
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   ClearCurrentFrame(hWnd);
   return TRUE;
}

//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//  PURPOSE:  Processes messages for the main window.
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) 
	{
		case WM_CREATE:
		{
			char tLine[101];
			globalData.magnification = 1.0;
			globalData.isCyclic = TRUE;
			globalData.fps = 16;
			
			ReadCurrentSprite(spriteName);
			if (gCurrentSprite)
				sprintf(tLine, "HLSV: %s", spriteName);
			else
				strcpy(tLine, "HL Sprite viewer");
			SetWindowText(hWnd, tLine);
			InitializeDirectDraw(hWnd);
				
			StartDrawTimer(hWnd);
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_OPEN:
					if (DoOpenFile(hWnd))
					{
 						ResizeMainWindow(hWnd);
						ClearCurrentFrame(hWnd);
					}
					break;
			
				case IDM_SAVEIMAGE:
					DoSaveFile(hWnd);
					break;

				case IDM_SAVESEQUENCE:
					DoSaveSequence(hWnd);
					break;

				case IDM_ABOUT:
				   DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)AboutDlgProc);
				   break;

				case IDM_SPEED:
				   DialogBox(hInst, (LPCTSTR)IDD_SPEED, hWnd, (DLGPROC)SpeedDlgProc);
				   break;

				case IDM_INFO:
				   DialogBox(hInst, (LPCTSTR)IDD_INFO, hWnd, (DLGPROC)InfoDlgProc);
				   break;

				case IDM_STEP:
				{
					globalData.playOnce = FALSE;
					globalData.isCyclic = FALSE;
					CycleFrame(hWnd);
					break;
				}

				case IDM_PLAYONCE:
				{
					globalData.playOnce = TRUE;
					globalData.isCyclic = TRUE;
					gCurrentFrame = gCurrentSprite->frames;
					break;
				}

				case IDM_ZOOMIN:
				{
					if (globalData.magnification < 16)
						globalData.magnification *= 2;
 					ResizeMainWindow(hWnd);
					ClearCurrentFrame(hWnd);
					break;
				}

				case IDM_ZOOMOUT:
				{
					if (globalData.magnification > (1.0 / 16.0))
						globalData.magnification /= 2;
					ResizeMainWindow(hWnd);
					ClearCurrentFrame(hWnd);
					break;
				}

				case IDM_CYCLE:
				{
					globalData.playOnce = FALSE;
					globalData.isCyclic = TRUE;
					CycleFrame(hWnd);
					break;
				}

				case IDM_EXIT:
				   DestroyWindow(hWnd);
				   break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_PAINT:
		{
			HDC theDC;
			PAINTSTRUCT ps;

			ClearCurrentFrame(hWnd);
			theDC = BeginPaint(hWnd, &ps);
			if (gCurrentSprite)
				DisplayFrame(hWnd, theDC, globalData.isCyclic);
			else
			{
				RECT			theRect;
				HBRUSH			theBrush;
				HPEN			thePen;

				GetClientRect(hWnd, &theRect);

				theRect.left = theRect.top = 0;
				theBrush = (HBRUSH)SelectObject(theDC, GetStockObject(WHITE_BRUSH));
				thePen = (HPEN)SelectObject(theDC, GetStockObject(NULL_PEN));
				Rectangle(theDC, theRect.left, theRect.top, theRect.right, theRect.bottom);
				DeleteObject(SelectObject(theDC, theBrush));
				DeleteObject(SelectObject(theDC, thePen));
			}
			EndPaint(hWnd, &ps);
				
			break;
		}

		case WM_ERASEBKGND:
		{
			HDC		theDC = (HDC)wParam;
			HBRUSH	theBrush;
			RECT	theRect;

			theBrush = (HBRUSH)SelectObject(theDC, GetStockObject(WHITE_BRUSH));
			GetWindowRect(hWnd, &theRect);
			Rectangle(theDC, theRect.left, theRect.top, theRect.right, theRect.bottom);
			DeleteObject(SelectObject(theDC, theBrush));
			break;
		}

		case WM_DESTROY:
			StopDrawTimer(hWnd);
			DisposeDirectDraw();
			DisposeCurrentSprite();
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}
