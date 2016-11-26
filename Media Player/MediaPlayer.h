#pragma once
#include "MediaCallback.h"

typedef struct MediaWindow
{
	HWND MediahWnd;
	HWND ToolbarhWnd;
	HWND DevicehWnd;
	HWND FormathWnd;
	HWND VideohWnd;

	LONG ToolbarHeight;
	int DeviceIndex;
	int FormatIndex;
}MediaWindow;

typedef struct MediaPlayer
{
	MediaDeviceSet DeviceSet;
	MediaAsyncCallback AsyncCallback;
	MediaWindow Window;
}MediaPlayer;

BOOL MediaRegisterClass(HINSTANCE hInstance);
BOOL MediaPlayerCreate(MediaPlayer *pPlayer, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HINSTANCE hInstance);
