
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>
#include "gamecode.h"

#pragma comment(lib, "winmm.lib")

const LPSTR WINDOW_CLASS_NAME = "game_main";
const int SCREEN_HALF_WIDTH = GetSystemMetrics(SM_CXSCREEN) >> 1;
const int SCREEN_HALF_HEIGHT = GetSystemMetrics(SM_CYSCREEN) >> 1;
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

HWND hwndWindow;
bool bAppActive = true;

int	 g_iScreenWidth;
int	 g_iScreenHeight;
int	 g_iRefreshRate;
int  g_iAALevel;
bool g_bFullScreen;
char g_szMapName[1024];

byte keys[256];

int round(double v)
{
	int n = (int)v;
	return (v-n>=0.5f) ? n+1 : n;
}

//WindowProc///////////////////////////////////////////////////////////////////
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	PAINTSTRUCT ps;
	HDC hdc;

	switch(msg) {
		case WM_CREATE:
			{
				return 0;
			}break;
		case WM_ACTIVATE:
			{
				bAppActive = (wparam == WA_ACTIVE)||
					(wparam == WA_CLICKACTIVE);
			}break;
		case WM_PAINT:
			{
				hdc = BeginPaint(hwnd, &ps);
				EndPaint(hwnd, &ps);
				return 0;
			}break;
		case WM_KEYDOWN: 
			{
				switch(wparam) {
					case VK_ESCAPE:
						{
							SendMessage(hwnd, WM_DESTROY, NULL, NULL);
						}break;
				}
			}break;
		case WM_DESTROY:
			{
				PostQuitMessage(0);
				return 0;
			}break;
	}//case
		return DefWindowProc(hwnd, msg, wparam, lparam);
}//WindowProc

//WinMain//////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, int nCmdShow) 
{
	//read config from file
	char szAppName[256];
	memset(szAppName, 0, 256);
	strcpy(szAppName, "Game settings\0");

	char szFileName[1024];
	memset(szFileName, 0, 1024);
	GetModuleFileName(NULL, szFileName, 1024);

	char *c_ptr = &szFileName[strlen(szFileName)-1];
	int cnt = strlen(szFileName);
	while(*c_ptr != '\\')
	{
		c_ptr--;
		cnt--;
	}
	*c_ptr = '\0';
	strcat(szFileName, "\\settings.ini\0");

	GetPrivateProfileString(szAppName, "Map", "default.map",
		g_szMapName, 1024, szFileName);

	memset(szAppName, 0, 256);
	strcpy(szAppName, "Display settings\0");
	
	g_iScreenWidth = GetPrivateProfileInt(szAppName, "Screen_width", 
		800, szFileName);
	g_iScreenHeight = GetPrivateProfileInt(szAppName, "Screen_height",
		600, szFileName);
	g_iRefreshRate = GetPrivateProfileInt(szAppName, "Refresh_rate",
		75, szFileName);
	g_iAALevel = GetPrivateProfileInt(szAppName, "Antialiasing_level",
		0, szFileName);
	g_bFullScreen = GetPrivateProfileInt(szAppName, "Run_fullscreen",
		1, szFileName);
	
	srand(timeGetTime());
	
	WNDCLASSEX winclass;
	MSG msg;

	ZeroMemory(&winclass, sizeof(winclass));
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_HREDRAW | CS_VREDRAW;
	winclass.lpfnWndProc = WindowProc;
	winclass.hInstance = hInstance;
	winclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winclass.lpszClassName = WINDOW_CLASS_NAME;

	if (!RegisterClassEx(&winclass)) return 0;
	if (!(hwndWindow = CreateWindowEx(NULL, WINDOW_CLASS_NAME, "game console",
								WS_POPUP|WS_CAPTION, 
								SCREEN_HALF_WIDTH - (g_iScreenWidth >> 1), 
								SCREEN_HALF_HEIGHT - (g_iScreenHeight >> 1), 
								g_iScreenWidth, g_iScreenHeight,
								NULL, NULL, hInstance, NULL))) return 0;
	
	SetForegroundWindow(hwndWindow);
	ShowWindow(hwndWindow, SW_SHOW);
	UpdateWindow(hwndWindow);
	
	HRESULT hr ;
	if(FAILED(hr = InitD3D(hwndWindow)))
	{
		ErrorMessage(hwndWindow, hr);
		return 0;
	}

	ShowSplash();

	if(FAILED(hr = InitDI(hwndWindow, hInstance)))
	{
		ErrorMessage(hwndWindow, hr);
		return 0;
	}
	
	memset(szFileName, 0, 1024);
	strcpy(szFileName, "data\\maps\\");
	strcat(szFileName, g_szMapName);
	if(FAILED(hr = LoadMap(szFileName)))
	{
		ErrorMessage(hwndWindow, hr);
		return 0;
	}
	
	if(FAILED(hr = LoadGameData()))
	{
		ErrorMessage(hwndWindow, hr);
		return 0;
	}

	SetActiveWindow(hwndWindow);
	ShowCursor(SW_HIDE);

	DWORD LastTickCount = GetTickCount();
	DWORD Delta;

	int hsx = GetSystemMetrics(SM_CXSCREEN)>>1;
	int hsy = GetSystemMetrics(SM_CYSCREEN)>>1;

	//Message cycle
	while(true)
	{
		Delta = GetTickCount() - LastTickCount;
		LastTickCount = GetTickCount();
		if(PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE))
			if(GetMessage(&msg, NULL, NULL, NULL))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
				break;
		if(bAppActive)
		{
			SetCursorPos(hsx, hsy);

			hr = UpdateScene(Delta);
			if(hr == DIERR_INPUTLOST)
				hr = RestoreDI();	
			
			if(FAILED(hr))
			{
				ErrorMessage(hwndWindow, hr);
				return 0;

			}

			hr = UpdateFrame();
			if(hr == D3DERR_DEVICELOST)
				hr = RestoreD3D();

			if(FAILED(hr))
			{
				ErrorMessage(hwndWindow, hr);
				return 0;
			}
		}
		
	}
	Cleanup();
		
	return 0;
}//WinMain